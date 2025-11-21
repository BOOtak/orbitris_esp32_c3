#pragma once

#include <cstdint>

#include "game_utils.h"
#include "orbital.h"
#include "screen.h"
#include "sharp_display.h"
#include "tetramino.h"

#define TILES_X 10
#define TILES_Y 10

struct Tile {
  bool occupied;
};

class GameScreen : public Screen {
public:
  GameScreen();

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() override;

private:
  Tile tilemap_[TILES_Y][TILES_X]{};
  ActiveTetramino active_tetramino_;
  ActiveTetramino next_tetramino_;
  ActiveTetramino sliding_tetramino_;
  PlanetState planet_state_;
  Vector2 star_pos_{ LCD_WIDTH / 2, LCD_HEIGHT / 2 };
  float delta_time_;

  bool is_blank(Tile& tile);
  void draw_tilemap();
  int get_resolution(const PlanetState& planet);
  Rectangle intersect_tiles(const ActiveTetramino& block);

  void get_tetramino_tilemap_pos_corner(const ActiveTetramino& block, int* x, int* y);
  void get_tetramino_tilemap_pos(const ActiveTetramino& block, int (*coords)[2]);
  void place_tetramino(const ActiveTetramino& block);

  void reset_planet_state();
  void generate_next_tetramino();
};
