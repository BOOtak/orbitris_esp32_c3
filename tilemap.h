#pragma once

#include "game_utils.h"

constexpr auto TILES_X = 20;
constexpr auto TILES_Y = 20;

struct Tile {
  bool occupied;
};



class Tilemap {
public:
  Tilemap();

  void init();

  void draw();

  Rectangle intersect_tiles(const ActiveTetramino& block);

  void place_tetramino(const ActiveTetramino& block);

private:
  Tile tilemap_[TILES_Y][TILES_X]{};

  bool is_blank(const Tile& tile);

  void get_tetramino_tilemap_pos_corner(const ActiveTetramino& block, int* x, int* y);

  void get_tetramino_tilemap_pos(const ActiveTetramino& block, int (*coords)[2]);
};
