#pragma once

#include "game_utils.h"
#include "screen.h"

class HouseScreen : public Screen {
public:
  HouseScreen();

  void init() override;

  Screen* update() override;

  void draw() const override;

  void close() override;

private:
  void update_camera();

  Vector2 player_pos_{};
  Vector2 camera_pos_{};
  float mess_{};
  bool lost_{};
  bool won_{};
  int carried_index_{ -1 };
};
