#pragma once

#include "highscore.h"
#include "screen.h"
#include "game_utils.h"

class HighscoreTableScreen : public Screen {
public:
  HighscoreTableScreen(HighscoreTable& highscores);

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

private:
  HighscoreTable& highscores_;
  static constexpr int VISIBLE_ENTRIES = 8;  // Show all 8 entries
};
