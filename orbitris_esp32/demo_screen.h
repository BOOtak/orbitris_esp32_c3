#pragma once

#include "game_utils.h"
#include "screen.h"

class DemoScreen : public Screen {
public:
  DemoScreen();

  void init() override;

  Screen* update() override;

  void draw() const override;

  void close() override;

private:
  void update_camera();

  Vector2 player_pos_{};
  Vector2 camera_pos_{};
};
