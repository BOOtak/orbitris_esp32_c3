#include "draw.h"

#include <cstdlib>

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

void draw_rectangle_lines_pattern(const Rectangle &rect, uint8_t pattern_size, uint8_t pattern) {
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
