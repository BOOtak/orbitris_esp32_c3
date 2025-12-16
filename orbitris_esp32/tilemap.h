#pragma once

#include <cstdlib>

#include "game_utils.h"
#include "tetramino.h"

constexpr auto TILES_X = 20;
constexpr auto TILES_Y = 20;
constexpr auto ROW_LENGTH = 8;
constexpr auto DEATH_LENGTH = 12;

enum class TileFlags : uint8_t {
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

  bool tile_out_of_bounds{};

  Tilemap();

  void init();

  void update();

  void draw() const;

  Rectangle intersect_tiles(const ActiveTetramino& block);

  void place_tetramino(const ActiveTetramino& block);

  bool can_move(const ActiveTetramino& block, int dx, int dy) const;

  /**
   * @brief Convert ActiveTetramino screen coordinates into tilemap row and column indices of the topleft corner of the ActiveTetramino
   *
   * @param block Tetramino block to convert coordinates of
   * @param[out] x Column index of the block in the tilemap
   * @param[out] y Row index of the block in the tilemap
   */
  void get_tetramino_tilemap_pos_corner(const ActiveTetramino& block, int& x, int& y) const;

  /**
   * @brief Convert tile indices into screen coordinates
   *
   * @param ix Column index of the tile in tilemap
   * @param iy Row index of the tile in tilemap
   * @return Screen coordinates of the tile
   */
  Vector2 get_tile_pos(int ix, int iy) const;

private:
  Tile tilemap_[TILES_Y][TILES_X]{};
  TileDeleteInfo tile_delete_info_{};

  bool is_blank(const Tile& tile) const;

  void check_rows();

  void check_bounds();

  void check_and_flag_oob(size_t i, size_t j);

  void get_tetramino_tilemap_pos(const ActiveTetramino& block, int (*coords)[2]) const;

  void delete_tiles_for_real();
};
