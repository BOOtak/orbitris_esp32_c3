#pragma once

#include "highscore.h"
#include "screen.h"
#include "stats.h"
#include "game_utils.h"

constexpr auto bufsize = 100;

class GameOverScreen : public Screen {
public:
  GameOverScreen(Stats& stats, HighscoreTable& highscores);

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

private:
  Stats& stats_;
  HighscoreTable& highscores_;
  Vector2 text_size_;
  char score_buffer_[bufsize];
};
