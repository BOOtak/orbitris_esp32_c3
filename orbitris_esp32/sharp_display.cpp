#include "sharp_display.h"

#include <Arduino.h>
#include <SPI.h>

#include "const.h"

#define BYTES_PER_LINE (LCD_WIDTH / 8)  // 50 bytes per line

// Pin Definitions
#define PIN_NUM_MOSI 6  // Data (DI)
#define PIN_NUM_CLK 4   // Clock (CLK)
#define PIN_NUM_CS 21   // Chip Select (CS) - controlled manually

// LCD Commands
#define CMD_UPDATE_MODE 0x80  // Write Command: 1000b + VCOM bit (bit 6)
#define CMD_ALL_CLEAR 0x20    // All Clear (0010 0000b)
#define CMD_NOP 0x00          // No Operation (Trailer Byte)

#define LINE_PREFIX_LENGTH 2
// The entire transaction buffer for one line:
// [1 byte Command] + [1 byte Line Address] + [BYTES_PER_LINE data] + [1 byte Trailer]
#define LINE_LENGTH (LINE_PREFIX_LENGTH + BYTES_PER_LINE)
#define UPDATE_COMMAND_SUFFIX_LENGTH 2

#define TO_BE(x) ((x << 7) | ((x & 0x02) << 5) | ((x & 0x04) << 3) | ((x & 0x08) << 1) | ((x & 0x10) >> 1) | ((x & 0x20) >> 3) | ((x & 0x40) >> 5) | (x >> 7))

// Global Variables
static uint8_t framebuffer[LINE_LENGTH * LCD_HEIGHT + UPDATE_COMMAND_SUFFIX_LENGTH];
static int vcom_state = 0;  // 0 or 1 for VCOM polarity

// SPI settings for the Sharp LCD (datasheet specifies 2MHz, overclocking to 10MHz)
static SPISettings sharpLcdSettings(10000000, MSBFIRST, SPI_MODE0);


/**
 * @brief Pre-calculate commands and line numbers for the framebuffer
 */
void framebuffer_init() {
  for (int y = 0; y < LCD_HEIGHT; y++) {
    int line_number_idx = (y * LINE_LENGTH) + 1;
    framebuffer[line_number_idx] = TO_BE(y + 1);
  }
}

/**
 * @brief Toggles the VCOM hardware pin and updates the VCOM state for the next command.
 */
void lcd_toggle_vcom() {
  vcom_state = !vcom_state;
}

/**
 * @brief Transfers data via SPI using the Arduino SPI library.
 * @param buffer The data buffer to send.
 * @param len The length of the buffer in bytes.
 */
void spi_transfer_bytes(const uint8_t* buffer, size_t len) {
  SPI.beginTransaction(sharpLcdSettings);
  digitalWrite(PIN_NUM_CS, HIGH);

  SPI.transferBytes((uint8_t*)buffer, nullptr, len);

  digitalWrite(PIN_NUM_CS, LOW);
  SPI.endTransaction();
}

void lcd_init() {
  pinMode(PIN_NUM_CS, OUTPUT);

  // Initialize SPI bus (CLK, MOSI). MISO and SS are set to -1 as they are not used.
  SPI.begin(PIN_NUM_CLK, -1, PIN_NUM_MOSI, -1);

  // Set initial states
  digitalWrite(PIN_NUM_CS, LOW);  // CS Low (Inactive)
  vcom_state = 0;

  framebuffer_init();
}

void lcd_update() {
  // 1. Send framebuffer data
  framebuffer[0] = CMD_UPDATE_MODE | (vcom_state << 6);
  spi_transfer_bytes(framebuffer, sizeof(framebuffer));

  // 2. Toggle VCOM polarity for the next frame
  lcd_toggle_vcom();
}

void lcd_clear() {
  uint8_t command[2];

  // Command Byte: CMD_ALL_CLEAR (0x20) | VCOM state (bit 6)
  command[0] = CMD_ALL_CLEAR | (vcom_state << 6);

  // Trailer Byte (NOP)
  command[1] = CMD_NOP;

  spi_transfer_bytes(command, 2);

  // After All Clear, the VCOM polarity should be toggled for the next frame
  lcd_toggle_vcom();
}

void lcd_fill_buffer(int color) {
  // Sharp LCD logic: 0 = White (Clear), 1 = Black (Set)
  uint8_t value = (color == 1) ? 0xFF : 0x00;
  for (int y = 0; y < LCD_HEIGHT; y++) {
    int byte_index = (y * LINE_LENGTH) + LINE_PREFIX_LENGTH;
    memset(&framebuffer[byte_index], value, BYTES_PER_LINE);
  }
}

void lcd_draw_pixel(int x, int y, int color) {
  if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) return;

  // Calculate byte index and bit position
  int byte_index = (y * LINE_LENGTH) + LINE_PREFIX_LENGTH + (x >> 3);
  // Display is big-endian, so left-to-right is MSB (7) to LSB (0)
  uint8_t bit_pos = 7 - (x & 7);

  if (color == 1) {
    // Set bit (Black/Pixel On)
    framebuffer[byte_index] |= (1 << bit_pos);
  } else {
    // Clear bit (White/Pixel Off)
    framebuffer[byte_index] &= ~(1 << bit_pos);
  }
}
