#include "game_over_screen.h"

#include <cstdio>

#include "const.h"
#include "draw.h"
#include "input.h"
#include "screen.h"

GameOverScreen::GameOverScreen(const Stats &stats) : Screen(), stats_{ stats } {
}

void GameOverScreen::init() {
  snprintf(score_buffer_, bufsize, "Game Over!\nScore: %d", stats_.game_points);
  text_size_ = measure_text(score_buffer_, 2);
}

Screen *GameOverScreen::update() {
  if (is_key_pressed(ESP_KEY_A)) {
    return screens::game_screen;
  }

  return this;
}

void GameOverScreen::draw() const {
  fill_scrfeen_buffer(1);

  const int text_x = LCD_WIDTH / 2 - text_size_.x / 2;
  const int text_y = LCD_HEIGHT / 2 - text_size_.y / 2;

  print_text(text_x, text_y, 2, score_buffer_, 0);
}
