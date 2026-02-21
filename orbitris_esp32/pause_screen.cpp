#include "pause_screen.h"

#include <array>
#include <cstdio>

#include "input.h"
#include "const.h"
#include "draw.h"

PauseScreen::PauseScreen() {
  snprintf(text_buffer_, std::size(text_buffer_), "Pause");
  text_size_ = measure_text(text_buffer_, 2);
}

void PauseScreen::init() {
}

Screen *PauseScreen::update() {
  if (is_key_pressed(ESP_KEY_A)) {
    return screens::game_screen;
  }

  return this;
}

void PauseScreen::draw() const {
  fill_scrfeen_buffer(1);
  // draw_rectangle_checkerboard(0, 0, LCD_WIDTH, LCD_HEIGHT);

  const int text_x = LCD_WIDTH / 2 - text_size_.x / 2;
  const int text_y = LCD_HEIGHT / 2 - text_size_.y / 2;

  print_text(text_x, text_y, 2, text_buffer_, 0);
}
