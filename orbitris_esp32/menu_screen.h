#pragma once

#include "button.h"
#include "button_grid_manager.h"
#include "screen.h"

constexpr int BUTTONS_COUNT = 3;

class MenuScreen : public Screen {
public:
  MenuScreen();

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

private:

  Button menu_buttons_[BUTTONS_COUNT];
  int grid_map_[BUTTONS_COUNT];
  ButtonGridManager manager_;
};
