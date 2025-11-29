#pragma once

#include <cstdint>

#include "const.h"
#include "game_utils.h"
#include "orbital.h"
#include "screen.h"
#include "tetramino.h"
#include "tilemap.h"

class GameScreen : public Screen {
public:
  GameScreen();

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() override;

private:
  Tilemap tilemap_{};
  ActiveTetramino active_tetramino_;
  ActiveTetramino next_tetramino_;
  ActiveTetramino sliding_tetramino_;
  PlanetState planet_state_;
  Vector2 star_pos_{ LCD_WIDTH / 2, LCD_HEIGHT / 2 };
  float delta_time_;

  int get_resolution(const PlanetState& planet);

  void reset_planet_state();
  void generate_next_tetramino();
  void draw_trajectory();
};
