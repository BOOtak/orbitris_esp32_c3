#pragma once

#include "screen.h"
#include "button_grid_manager.h"

constexpr auto KEYS_COUNT = 40;

class HighscoreInputScreen : public Screen {
public:
  HighscoreInputScreen();

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

private:
  Button keyboard_[KEYS_COUNT];
  ButtonGridManager manager_;
};
