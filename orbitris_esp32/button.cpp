#include "button.h"

#include "const.h"
#include "draw.h"

void Button::draw() const {
  int x = rect.x;
  int y = rect.y;
  int w = rect.width;
  int h = rect.height;

  switch (state) {
    case ButtonState::idle:
      draw_rectangle_lines(x, y, w, h, LCD_BLACK);
      break;
    case ButtonState::focused:
      draw_rectangle_checkerboard(x + 1, y + 1, w - 2, h - 2);
      break;
    case ButtonState::pressed:
      draw_rectangle_checkerboard(x + 3, y + 3, w - 6, h - 6);
      break;
  }

  if (label) {
    Vector2 measure = measure_text(label, text_scale);
    int text_x = x + (w - measure.x) / 2;
    int text_y = y + (h - measure.y) / 2;
    print_text(text_x, text_y, text_scale, label, LCD_BLACK);
  }
}
