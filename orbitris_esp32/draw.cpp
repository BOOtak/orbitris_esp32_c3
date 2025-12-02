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

constexpr float ZOOM_EPSILON = 0.01f;

static bool g_should_scale = false;
static float g_scale = 1.0f;

extern void lcd_draw_pixel(int x, int y, int color);

extern void lcd_fill_buffer(int color);

void begin_scale(float scale) {
  if (fabsf(scale - 1.0f) > ZOOM_EPSILON) {
    g_should_scale = true;
    g_scale = scale;
  }
}

void end_scale() {
  g_should_scale = false;
  g_scale = 1.0f;
}

void draw_pixel(int x, int y, int color) {
  if (g_should_scale) {
    x = ((x - CENTER_X) * g_scale) + CENTER_X;
    y = ((y - CENTER_Y) * g_scale) + CENTER_Y;
  }

  lcd_draw_pixel(x, y, color);
}

void fill_scrfeen_buffer(int color) {
  lcd_fill_buffer(color);
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
      lcd_draw_pixel(i, j, color);
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
    lcd_draw_pixel(x, posY, color);
  }

  // Draw bottom horizontal line
  for (int x = posX; x < posX + width; x++) {
    lcd_draw_pixel(x, posY + height - 1, color);
  }

  // Draw left vertical line
  for (int y = posY; y < posY + height; y++) {
    lcd_draw_pixel(posX, y, color);
  }

  // Draw right vertical line
  for (int y = posY; y < posY + height; y++) {
    lcd_draw_pixel(posX + width - 1, y, color);
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
    lcd_draw_pixel(x, ry, get_color());
  }

  for (int y = ry; y < ry + rh; y++) {
    lcd_draw_pixel(rx + rw - 1, y, get_color());
  }

  for (int x = rx + rw - 1; x >= rx; x--) {
    lcd_draw_pixel(x, ry + rh - 1, get_color());
  }

  for (int y = ry + rh - 1; y >= ry; y--) {
    lcd_draw_pixel(rx, y, get_color());
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
    lcd_draw_pixel(x0, y0, color);
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
    lcd_draw_pixel(x0, y0, color);
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

  for (int col = 0; col < FONT_CHAR_WIDTH; ++col) {
    uint8_t column_byte = char_data[col];

    for (int row = 0; row < FONT_CHAR_HEIGHT; ++row) {
      if (column_byte & (1 << row)) {

        int px_x = draw_x + col * scale;
        int px_y = draw_y + row * scale;

        // Scale the pixel block
        for (int i = 0; i < scale; ++i) {
          for (int j = 0; j < scale; ++j) {
            int final_x = px_x + i;
            int final_y = px_y + j;

            // Bounds check
            if (final_x >= 0 && final_x < LCD_WIDTH && final_y >= 0 && final_y < LCD_HEIGHT) {
              lcd_draw_pixel(final_x, final_y, color);
            }
          }
        }
      }
    }
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