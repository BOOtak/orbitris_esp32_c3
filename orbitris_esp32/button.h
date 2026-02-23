#pragma once

#include "game_utils.h"

enum class ButtonState {
  idle,
  focused,
  pressed
};

struct Button {
  Rectangle rect;
  const char* label;
  int text_scale;
  int id;
  ButtonState state{ ButtonState::idle };

  void draw() const;
};
