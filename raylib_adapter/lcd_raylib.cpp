#include "raylib.h"

#include "../orbitris_esp32/const.h"
#include <cstdint>

extern RenderTexture2D target;

void lcd_draw_pixel(int x, int y, int color) {
  DrawPixel(x, y, color == 1 ? WHITE : BLACK);
}

void lcd_fill_buffer(int color) {
  ClearBackground(color == 1 ? WHITE : BLACK);
}

void lcd_fill_line(int line, uint8_t pattern, int color) {
  for (int i = 0; i < LCD_WIDTH; i++) {
    if (pattern & (0x80 >> (i & 7))) {
      DrawPixel(i, line, color == 1 ? WHITE : BLACK);
    }
  }
}
