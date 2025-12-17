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
}

Screen *GameOverScreen::update() {
  if (is_key_pressed(ESP_KEY_A)) {
    return screens::game_screen;
  }

  return this;
}

void GameOverScreen::draw() const {
  fill_scrfeen_buffer(1);
  constexpr int text_x = 150;
  constexpr int text_y = 112;

  Rectangle rect{ text_x - 2, text_y - 2, 102, 34 };
  draw_rectangle(rect, 1);
  print_text(text_x, text_y, 2, score_buffer_, 0);
}
