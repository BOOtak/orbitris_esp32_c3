#pragma once

#include <array>

#include "screen.h"
#include "button_grid_manager.h"
#include "highscore.h"
#include "stats.h"

namespace {
constexpr auto KEYS_COUNT = 40;
}

class HighscoreInputScreen : public Screen {
public:
  HighscoreInputScreen(const Stats& stats, HighscoreTable& highscores);

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

private:
  const Stats& stats_;
  HighscoreTable& highscores_;
  Button keyboard_[KEYS_COUNT];
  ButtonGridManager manager_;
  std::array<char, HIGHSCORE_NAME_SIZE> name_;
  int cursor_position_;
};
