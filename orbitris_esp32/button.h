#pragma once

#include "game_utils.h"

enum class ButtonState {
  idle,
  focused,
  pressed
};

class Button {
public:
  Button(Rectangle rect, const char* label, int scale, int id);

  void set_state(ButtonState new_state);

  const Rectangle& get_rect() const;

  int get_id() const;

  void draw() const;

private:
  Rectangle rect_;
  const char* label_;
  int text_scale_;
  int id_;
  ButtonState state_;
};
