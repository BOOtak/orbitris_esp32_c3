#include "draw.h"

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

extern void lcd_draw_pixel(int x, int y, int color);

extern void lcd_fill_buffer(int color);

void fill_scrfeen_buffer(int color) {
  lcd_fill_buffer(color);
}

void draw_rectangle_lines(int posX, int posY, int width, int height, int color) {
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
  int pattern_state = 0;
  auto get_color = [&pattern_state, pattern, pattern_size]() {
    return (pattern >> (7 - (pattern_state++ % pattern_size))) & 1;
  };

  for (int x = rect.x; x < rect.x + rect.width; x++) {
    lcd_draw_pixel(x, rect.y, get_color());
  }

  for (int y = rect.y; y < rect.y + rect.height; y++) {
    lcd_draw_pixel(rect.x + rect.width - 1, y, get_color());
  }

  for (int x = rect.x + rect.width - 1; x >= rect.x; x--) {
    lcd_draw_pixel(x, rect.y + rect.height - 1, get_color());
  }

  for (int y = rect.y + rect.height - 1; y >= rect.y; y--) {
    lcd_draw_pixel(rect.x, y, get_color());
  }
}

void draw_line(int x0, int y0, int x1, int y1, int color) {
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