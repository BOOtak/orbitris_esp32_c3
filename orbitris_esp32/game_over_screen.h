#pragma once

#include "screen.h"
#include "stats.h"

constexpr auto bufsize = 100;

class GameOverScreen : public Screen {
public:
  GameOverScreen(const Stats& stats);

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

private:
  const Stats& stats_;
  char score_buffer_[bufsize];
};
