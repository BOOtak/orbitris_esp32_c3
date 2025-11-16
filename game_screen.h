#pragma once

#include <cstdint>

#include "game_types.h"
#include "orbital.h"
#include "screen.h"
#include "sharp_display.h"

#define TILES_X 10
#define TILES_Y 10

struct Tile {
  bool occupied;
};

struct Tetramino {
  int data[4][4][4];
};

struct ActiveTetramino {
  int rot_index;
  Vector2 pos;
  Tetramino* block;
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
  PlanetState planet_state_;
  Vector2 star_pos_{LCD_WIDTH / 2, LCD_HEIGHT / 2};
  float delta_time_;


  bool is_blank(Tile& tile);
  void draw_tetramino(ActiveTetramino tetramino);
  void draw_tilemap();
  void update_delta_time();
};
