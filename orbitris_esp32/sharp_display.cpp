#include "sharp_display.h"

#include <Arduino.h>
#include <driver/spi_master.h>

#include <cstdint>
#include <cstring>
#include <utility>

#include "const.h"

// One DMA transaction per frame. Two framebuffers alternate: the game draws
// into one while DMA reads the other. At lcd_update(), the completed frame is
// queued for DMA and the buffers swap; the old frame is copied forward so
// incremental drawing works.
namespace {

constexpr int PIN_NUM_MOSI = 6;
constexpr int PIN_NUM_CLK = 4;
constexpr int PIN_NUM_CS = 21;

constexpr int SPI_CLOCK_HZ = 10000000;  // Datasheet says 2MHz; 10MHz works reliably.

constexpr uint8_t CMD_UPDATE_MODE = 0x80;
constexpr uint8_t CMD_ALL_CLEAR = 0x20;
constexpr uint8_t CMD_NOP = 0x00;

constexpr int BYTES_PER_LINE = LCD_WIDTH / 8;
constexpr int LINE_PREFIX_BYTES = 2;
constexpr int LINE_BYTES = LINE_PREFIX_BYTES + BYTES_PER_LINE;
constexpr int FRAME_SUFFIX_BYTES = 2;
constexpr size_t FRAME_BYTES = static_cast<size_t>(LINE_BYTES) * LCD_HEIGHT + FRAME_SUFFIX_BYTES;

// Each line is [address][data...]. The next line's first byte doubles as this
// line's NOP trailer; the final line's trailer comes from the frame suffix.
constexpr size_t line_address_offset(int y) {
  return static_cast<size_t>(y) * LINE_BYTES + 1;
}

constexpr size_t line_data_offset(int y) {
  return static_cast<size_t>(y) * LINE_BYTES + LINE_PREFIX_BYTES;
}

// The Sharp display expects line addresses with reversed bit order.
constexpr uint8_t line_to_address(uint8_t line) {
  return static_cast<uint8_t>((line << 7) | ((line & 0x02) << 5) | ((line & 0x04) << 3) | ((line & 0x08) << 1) | ((line & 0x10) >> 1) | ((line & 0x20) >> 3) | ((line & 0x40) >> 5) | (line >> 7));
}

void report_error(const char* what, esp_err_t error) {
  Serial.printf("LCD: %s failed: %s (%d)\n", what, esp_err_to_name(error), static_cast<int>(error));
}

struct Framebuffer {
  // DMA reads this memory, so keep each buffer suitably aligned.
  alignas(32) uint8_t bytes[FRAME_BYTES];
};

static_assert(sizeof(Framebuffer) % 32 == 0, "Framebuffer stride must preserve DMA alignment");

class SharpDisplay {
public:
  SharpDisplay()
    : draw_buffer_(&framebuffers_[0]), send_buffer_(&framebuffers_[1]) {}

  void begin();
  void update();
  void clear();

  void set_byte(int cx, int y, uint8_t value);
  void fill_buffer(int color);
  void fill_line(int line, uint8_t pattern, int color);
  void draw_pixel(int x, int y, int color);

private:
  static void prepare(Framebuffer& framebuffer);
  bool wait_for_transfer();
  bool queue_frame();
  bool send_command(const uint8_t* data, size_t length);

  Framebuffer framebuffers_[2]{};
  Framebuffer* draw_buffer_;
  Framebuffer* send_buffer_;
  spi_device_handle_t spi_{};
  spi_transaction_t transaction_{};
  bool transfer_pending_{};
  uint8_t vcom_{};
};

SharpDisplay display{};

void SharpDisplay::prepare(Framebuffer& framebuffer) {
  std::memset(framebuffer.bytes, 0, FRAME_BYTES);
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    framebuffer.bytes[line_address_offset(y)] = line_to_address(static_cast<uint8_t>(y + 1));
  }
}

void SharpDisplay::begin() {
  if (spi_) {
    return;
  }

  prepare(framebuffers_[0]);
  prepare(framebuffers_[1]);
  draw_buffer_ = &framebuffers_[0];
  send_buffer_ = &framebuffers_[1];
  transfer_pending_ = false;
  vcom_ = 0;

  spi_bus_config_t bus_config = {};
  bus_config.mosi_io_num = PIN_NUM_MOSI;
  bus_config.miso_io_num = -1;
  bus_config.sclk_io_num = PIN_NUM_CLK;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  // One transaction covers the whole frame, keeping CS asserted throughout.
  bus_config.max_transfer_sz = static_cast<int>(FRAME_BYTES);

  esp_err_t error = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
  if (error != ESP_OK) {
    report_error("bus init", error);
    return;
  }

  // The Sharp LCD uses an active-high chip select.
  spi_device_interface_config_t device_config = {};
  device_config.clock_speed_hz = SPI_CLOCK_HZ;
  device_config.mode = 0;
  device_config.spics_io_num = PIN_NUM_CS;
  device_config.queue_size = 1;  // One frame in flight; the previous one is reaped first.
  device_config.flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_POSITIVE_CS;

  error = spi_bus_add_device(SPI2_HOST, &device_config, &spi_);
  if (error != ESP_OK) {
    report_error("device init", error);
    spi_ = nullptr;
  }
}

void SharpDisplay::update() {
  if (!spi_ || !wait_for_transfer()) {
    return;
  }

  draw_buffer_->bytes[0] = static_cast<uint8_t>(CMD_UPDATE_MODE | (vcom_ << 6));

  // The frame just drawn becomes the DMA source; the other buffer becomes the
  // next draw target. Copy the frame across so screens that draw incrementally
  // (pause / transitions) still start from the previously displayed image.
  std::swap(draw_buffer_, send_buffer_);
  std::memcpy(draw_buffer_->bytes, send_buffer_->bytes, FRAME_BYTES);

  if (queue_frame()) {
    vcom_ ^= 1;
  }
}

void SharpDisplay::clear() {
  if (!spi_ || !wait_for_transfer()) {
    return;
  }

  const uint8_t command[] = {
    static_cast<uint8_t>(CMD_ALL_CLEAR | (vcom_ << 6)),
    CMD_NOP,
  };

  if (send_command(command, sizeof(command))) {
    vcom_ ^= 1;
  }
}

bool SharpDisplay::wait_for_transfer() {
  if (!transfer_pending_) {
    return true;
  }

  spi_transaction_t* result = nullptr;
  esp_err_t error = spi_device_get_trans_result(spi_, &result, portMAX_DELAY);
  transfer_pending_ = false;

  if (error != ESP_OK) {
    report_error("DMA wait", error);
    return false;
  }
  return true;
}

bool SharpDisplay::queue_frame() {
  transaction_ = {};
  transaction_.length = FRAME_BYTES * 8;
  transaction_.tx_buffer = send_buffer_->bytes;

  // With one frame in flight and the previous result reaped, this queue cannot
  // be full. A failure here is a real driver/programming error, not something
  // a blocking retry could recover from.
  esp_err_t error = spi_device_queue_trans(spi_, &transaction_, 0);
  if (error != ESP_OK) {
    report_error("frame queue", error);
    return false;
  }

  transfer_pending_ = true;
  return true;
}

bool SharpDisplay::send_command(const uint8_t* data, size_t length) {
  spi_transaction_t transaction = {};
  transaction.length = length * 8;
  transaction.tx_buffer = data;

  esp_err_t error = spi_device_transmit(spi_, &transaction);
  if (error != ESP_OK) {
    report_error("command transfer", error);
    return false;
  }
  return true;
}

void SharpDisplay::set_byte(int cx, int y, uint8_t value) {
  draw_buffer_->bytes[line_data_offset(y) + cx] = value;
}

void SharpDisplay::fill_buffer(int color) {
  const uint8_t value = (color == 1) ? 0xFF : 0x00;
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    std::memset(draw_buffer_->bytes + line_data_offset(y), value, BYTES_PER_LINE);
  }
}

void SharpDisplay::fill_line(int line, uint8_t pattern, int color) {
  uint8_t* data = draw_buffer_->bytes + line_data_offset(line);
  for (int x = 0; x < BYTES_PER_LINE; ++x) {
    if (color == 1) {
      data[x] |= pattern;
    } else {
      data[x] &= static_cast<uint8_t>(~pattern);
    }
  }
}

void SharpDisplay::draw_pixel(int x, int y, int color) {
  if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) {
    return;
  }

  uint8_t& byte = draw_buffer_->bytes[line_data_offset(y) + (x >> 3)];
  const uint8_t mask = static_cast<uint8_t>(1u << (7 - (x & 7)));

  if (color == 1) {
    byte |= mask;
  } else {
    byte &= static_cast<uint8_t>(~mask);
  }
}
}  // namespace

void lcd_init() {
  display.begin();
}

void lcd_update() {
  display.update();
}

void lcd_clear() {
  display.clear();
}

void lcd_set_byte(int cx, int y, uint8_t value) {
  display.set_byte(cx, y, value);
}

void lcd_fill_buffer(int color) {
  display.fill_buffer(color);
}

void lcd_fill_line(int line, uint8_t pattern, int color) {
  display.fill_line(line, pattern, color);
}

void lcd_draw_pixel(int x, int y, int color) {
  display.draw_pixel(x, y, color);
}
