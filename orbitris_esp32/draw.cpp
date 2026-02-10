#include "draw.h"

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include "charmap.h"
#include "const.h"

constexpr int FONT_CHAR_WIDTH = 5;
constexpr int FONT_CHAR_HEIGHT = 8;
constexpr int FONT_START_CHAR = 0x20;
constexpr int FONT_END_CHAR = 0x7E;
constexpr int FONT_MAP_SIZE = FONT_END_CHAR - FONT_START_CHAR + 1;

constexpr float SCALE_EPSILON = 0.01f;

// Scaling
static bool g_should_scale = false;
static float g_draw_scale = 1.0f;
static float g_screen_scale = 1.0f;
static float g_scale = 1.0f;

// Draw masking
static bool g_should_mask = false;
static DrawMask g_draw_mask = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

extern void lcd_draw_pixel(int x, int y, int color);

extern void lcd_fill_buffer(int color);

extern void lcd_fill_line(int line, uint8_t pattern, int color);

static void update_scale() {
  g_scale = g_draw_scale * g_screen_scale;
  if (fabsf(g_scale - 1.0f) > SCALE_EPSILON) {
    g_should_scale = true;
  } else {
    g_should_scale = false;
  }
}

void begin_scale(float scale) {
  g_draw_scale = scale;
  update_scale();
}

void end_scale() {
  g_draw_scale = 1.0f;
  update_scale();
}

void begin_screen_scale(float scale) {
  g_screen_scale = scale;
  update_scale();
}

void end_screen_scale() {
  g_screen_scale = 1.0f;
  update_scale();
}

void begin_mask(DrawMask draw_mask) {
  g_should_mask = true;
  g_draw_mask = draw_mask;
}

void end_mask() {
  g_should_mask = false;
}

static void draw_pixel_masked(int x, int y, int color) {
  if (g_should_mask) {
    int masked_bit = 0x80 >> (x & 7);
    if (g_draw_mask.mask[y & 7] & masked_bit) {
      lcd_draw_pixel(x, y, color);
    }
  } else {
    lcd_draw_pixel(x, y, color);
  }
}

void draw_pixel(int x, int y, int color) {
  if (g_should_scale) {
    x = ((x - CENTER_X) * g_scale) + CENTER_X;
    y = ((y - CENTER_Y) * g_scale) + CENTER_Y;
  }

  draw_pixel_masked(x, y, color);
}

void fill_scrfeen_buffer(int color) {
  if (g_should_mask) {
    for (size_t i = 0; i < LCD_HEIGHT; i++) {
      lcd_fill_line(i, g_draw_mask.mask[i & 7], color);
    }
  } else {
    lcd_fill_buffer(color);
  }
}

void draw_rectangle(const Rectangle& rect, int color) {
  int rx, ry, rw, rh;
  if (g_should_scale) {
    rx = (rect.x - CENTER_X) * g_scale + CENTER_X;
    ry = (rect.y - CENTER_Y) * g_scale + CENTER_Y;
    rw = rect.width * g_scale;
    rh = rect.height * g_scale;
  } else {
    rx = rect.x;
    ry = rect.y;
    rw = rect.width;
    rh = rect.height;
  }

  for (int i = rx; i < rx + rw; i++) {
    for (int j = ry; j < ry + rh; j++) {
      draw_pixel_masked(i, j, color);
    }
  }
}

void draw_rectangle_checkerboard(int posX, int posY, int width, int height) {
  if (g_should_scale) {
    posX = (posX - CENTER_X) * g_scale + CENTER_X;
    posY = (posY - CENTER_Y) * g_scale + CENTER_Y;
    width = width * g_scale;
    height = height * g_scale;
  }

  for (int i = posX; i < posX + width; i++) {
    for (int j = posY; j < posY + height; j++) {
      if ((i + j) & 1) {
        draw_pixel_masked(i, j, 0);
      }
    }
  }
}

void draw_rectangle_lines(int posX, int posY, int width, int height, int color) {
  if (g_should_scale) {
    posX = (posX - CENTER_X) * g_scale + CENTER_X;
    posY = (posY - CENTER_Y) * g_scale + CENTER_Y;
    width = width * g_scale;
    height = height * g_scale;
  }

  // Draw top horizontal line
  for (int x = posX; x < posX + width; x++) {
    draw_pixel_masked(x, posY, color);
  }

  // Draw bottom horizontal line
  for (int x = posX; x < posX + width; x++) {
    draw_pixel_masked(x, posY + height - 1, color);
  }

  // Draw left vertical line
  for (int y = posY; y < posY + height; y++) {
    draw_pixel_masked(posX, y, color);
  }

  // Draw right vertical line
  for (int y = posY; y < posY + height; y++) {
    draw_pixel_masked(posX + width - 1, y, color);
  }
}

void draw_rectangle_lines_pattern(const Rectangle& rect, uint8_t pattern_size, uint8_t pattern) {
  int rx, ry, rw, rh;
  if (g_should_scale) {
    rx = (rect.x - CENTER_X) * g_scale + CENTER_X;
    ry = (rect.y - CENTER_Y) * g_scale + CENTER_Y;
    rw = rect.width * g_scale;
    rh = rect.height * g_scale;
  } else {
    rx = rect.x;
    ry = rect.y;
    rw = rect.width;
    rh = rect.height;
  }

  int pattern_state = 0;
  auto get_color = [&pattern_state, pattern, pattern_size]() {
    return (pattern >> (7 - (pattern_state++ % pattern_size))) & 1;
  };

  for (int x = rx; x < rx + rw; x++) {
    draw_pixel_masked(x, ry, get_color());
  }

  for (int y = ry; y < ry + rh; y++) {
    draw_pixel_masked(rx + rw - 1, y, get_color());
  }

  for (int x = rx + rw - 1; x >= rx; x--) {
    draw_pixel_masked(x, ry + rh - 1, get_color());
  }

  for (int y = ry + rh - 1; y >= ry; y--) {
    draw_pixel_masked(rx, y, get_color());
  }
}

void draw_line(int x0, int y0, int x1, int y1, int color) {
  if (g_should_scale) {
    x0 = ((x0 - CENTER_X) * g_scale) + CENTER_X;
    y0 = ((y0 - CENTER_Y) * g_scale) + CENTER_Y;
    x1 = ((x1 - CENTER_X) * g_scale) + CENTER_X;
    y1 = ((y1 - CENTER_Y) * g_scale) + CENTER_Y;
  }

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2, e2;

  for (;;) {
    draw_pixel_masked(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}

int draw_line_pattern(int x0, int y0, int x1, int y1, int pattern_state, uint8_t pattern_size, uint8_t pattern) {
  if (g_should_scale) {
    x0 = ((x0 - CENTER_X) * g_scale) + CENTER_X;
    y0 = ((y0 - CENTER_Y) * g_scale) + CENTER_Y;
    x1 = ((x1 - CENTER_X) * g_scale) + CENTER_X;
    y1 = ((y1 - CENTER_Y) * g_scale) + CENTER_Y;
  }

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2, e2;

  for (;;) {
    int color = (pattern >> (7 - (pattern_state++ % pattern_size))) & 1;
    draw_pixel_masked(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }

  return pattern_state;
}

/**
 * @brief Draws a single character bitmap to the LCD with scaling.
 */
void draw_char(int draw_x, int draw_y, int scale, uint8_t char_code, int color) {
  int index = char_code - FONT_START_CHAR;
  if (index < 0 || index >= FONT_MAP_SIZE) {
    index = 0;
  }

  const uint8_t* char_data = charMap[index];

  // Use fixed-point arithmetics (16.16)
  // TODO: use struct/unions?
  int32_t g_scale_fp = (int32_t)(g_scale * 65536);
  int total_scale = scale * g_scale_fp;

  int x_destination = (((draw_x - CENTER_X) * g_scale_fp) >> 16) + CENTER_X;
  int y_destination = (((draw_y - CENTER_Y) * g_scale_fp) >> 16) + CENTER_Y;

  int width_destination = (FONT_CHAR_WIDTH * total_scale) >> 16;
  int height_destination = (FONT_CHAR_HEIGHT * total_scale) >> 16;

  int32_t source_step_fp = (1 << 16) / scale;
  source_step_fp = ((int64_t)source_step_fp << 16) / g_scale_fp;

  int32_t source_col_fp = 0;
  for (int col = 0; col < width_destination; col++) {
    int source_col = source_col_fp >> 16;
    uint8_t column_byte = char_data[source_col];

    int32_t source_row_fp = 0;
    for (int row = 0; row < height_destination; row++) {
      int source_row = source_row_fp >> 16;

      if (column_byte & (1 << source_row)) {
        draw_pixel_masked(x_destination + col, y_destination + row, color);
      }
      source_row_fp += source_step_fp;
    }
    source_col_fp += source_step_fp;
  }
}

void print_text(int x, int y, int scale, const char* text, int color) {
  if (scale < 1) return;

  const int scaled_char_width = FONT_CHAR_WIDTH * scale;
  const int scaled_char_height = FONT_CHAR_HEIGHT * scale;

  int current_x = x;
  int current_y = y;

  for (const char* p = text; *p != '\0'; ++p) {
    char current_char = *p;

    if (current_char == '\n') {
      current_x = x;
      current_y += scaled_char_height;
      if (current_y + scaled_char_height > LCD_HEIGHT) break;
      continue;
    }

    if (current_y + scaled_char_height > LCD_HEIGHT) {
      break;
    }

    draw_char(current_x, current_y, scale, current_char, color);
    current_x += scaled_char_width;
  }
}
