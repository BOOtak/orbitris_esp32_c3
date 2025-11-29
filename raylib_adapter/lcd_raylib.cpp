#include "raylib.h"

extern RenderTexture2D target;

void lcd_draw_pixel(int x, int y, int color)
{
    DrawPixel(x, y, color == 1 ? WHITE : BLACK);
}

void lcd_fill_buffer(int color)
{
    ClearBackground(color == 1 ? WHITE : BLACK);
}
