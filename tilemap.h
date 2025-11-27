#pragma once

#include "game_utils.h"
#include "tetramino.h"

constexpr auto TILES_X = 20;
constexpr auto TILES_Y = 20;
constexpr auto ROW_LENGTH = 8;
constexpr auto DEATH_LENGTH = 12;

enum class TileFlags {
  NONE = 0,
  TO_DELETE,
  OOB
};

struct Tile {
  bool occupied;
  TileFlags flags;
};

struct TileDeleteInfo {
  bool rows[TILES_Y];
  bool columns[TILES_X];
  float draw_size;
  bool should_delete;
};

class Tilemap {
public:
  int game_points{};

  Tilemap();

  void init();

  void update();

  void draw();

  Rectangle intersect_tiles(const ActiveTetramino& block);

  void place_tetramino(const ActiveTetramino& block);

private:
  Tile tilemap_[TILES_Y][TILES_X]{};
  TileDeleteInfo tile_delete_info_{};

  bool is_blank(const Tile& tile);

  void check_rows();

  void get_tetramino_tilemap_pos_corner(const ActiveTetramino& block, int* x, int* y);

  void get_tetramino_tilemap_pos(const ActiveTetramino& block, int (*coords)[2]);

  void delete_tiles_for_real();
};
