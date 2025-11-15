#pragma once

#include <cstdint>

#include "screen.h"

class GameScreen : public Screen {
public:
  GameScreen();

  virtual Screen* update() override;

  virtual void draw() override;

private:
  uint32_t frame_ = 0;
};
