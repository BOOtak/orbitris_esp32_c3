#include "button.h"

#include "const.h"
#include "draw.h"

Button::Button(Rectangle rect, const char *label, int scale, int id)
  : rect_{ rect }, label_{ label }, text_scale_{ scale }, id_{ id }, state_{ ButtonState::idle } {}

void Button::set_state(ButtonState new_state) {
  state_ = new_state;
}

const Rectangle &Button::get_rect() const {
  return rect_;
}

int Button::get_id() const {
  return id_;
}

void Button::draw() const {
  int x = rect_.x;
  int y = rect_.y;
  int w = rect_.width;
  int h = rect_.height;

  switch (state_) {
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


  if (label_) {
    Vector2 measure = measure_text(label_, text_scale_);
    int text_x = x + (w - measure.x) / 2;
    int text_y = y + (h - measure.y) / 2;
    print_text(text_x, text_y, text_scale_, label_, LCD_BLACK);
  }
}
