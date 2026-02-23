#include "game_over_screen.h"

#include <cstdio>

#include "const.h"
#include "draw.h"
#include "input.h"
#include "persist.h"
#include "screen.h"

GameOverScreen::GameOverScreen(Stats &stats, HighscoreTable &highscores) : Screen(), stats_{ stats }, highscores_{ highscores } {
}

void GameOverScreen::init() {
  snprintf(score_buffer_, bufsize, "Game Over!\nScore: %d", stats_.game_points);
  text_size_ = measure_text(score_buffer_, 2);
  stats_.in_game = false;
  persist_stats(stats_);

  if (highscores_.is_highscore(stats_.game_points)) {
    // TODO: enter highscore on the dedicated screen
    highscores_.add_highscore("Nagibator", stats_.game_points);
    persist_highscores(highscores_);
  }
}

Screen *GameOverScreen::update() {
  if (is_key_pressed(ESP_KEY_A)) {
    return screens::menu_screen;
  }

  return this;
}

void GameOverScreen::draw() const {
  fill_scrfeen_buffer(1);

  const int text_x = LCD_WIDTH / 2 - text_size_.x / 2;
  const int text_y = LCD_HEIGHT / 2 - text_size_.y / 2;

  print_text(text_x, text_y, 2, score_buffer_, 0);
}
