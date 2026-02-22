#pragma once

#include <array>
#include <cstdlib>

#include "game_utils.h"
#include "tetramino.h"

constexpr auto TILES_X = 20;
constexpr auto TILES_Y = 20;
constexpr auto ROW_LENGTH = 8;
constexpr auto DEATH_LENGTH = 12;

constexpr auto BITS_IN_BYTE = 8;
constexpr auto TILEMAP_CELLS = TILES_X * TILES_Y;
constexpr auto TILEMAP_BYTES = (TILEMAP_CELLS >> 3) + (TILEMAP_CELLS & 7 ? 1 : 0);

struct TileBitmap {
  std::array<uint8_t, TILEMAP_BYTES> data;

  inline bool get_tile(int x, int y) const {
    if (x >= TILES_X || y >= TILES_Y) {
      return false;
    }

    auto tile_idx = y * TILES_X + x;
    auto byte_idx = tile_idx >> 3;
    auto bit_idx = tile_idx & 7;

    return (data[byte_idx] & (1 << bit_idx)) != 0;
  }

  inline void set_tile(int x, int y) {
    if (x >= TILES_X || y >= TILES_Y) {
      return;
    }

    auto tile_idx = y * TILES_X + x;
    auto byte_idx = tile_idx >> 3;
    auto bit_idx = tile_idx & 7;

    data[byte_idx] |= (1 << bit_idx);
  }
};

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
  bool populated;
  bool should_delete;
};

struct Stats;

class Tilemap {
public:
  int game_points{};

  bool tile_out_of_bounds{};

  Tilemap();

  void init(const Stats& init_stats);

  bool update();

  void draw() const;

  Rectangle intersect_tiles(const ActiveTetramino& block) const;

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

  bool is_blank(int ix, int iy) const;

  TileBitmap serialize() const;

private:
  Tile tilemap_[TILES_Y][TILES_X]{};
  TileDeleteInfo tile_delete_info_{};
  bool tilemap_changed_{};

  bool is_blank(const Tile& tile) const;

  void check_rows();

  void check_bounds();

  void check_and_flag_oob(size_t i, size_t j);

  void get_tetramino_tilemap_pos(const ActiveTetramino& block, int (*coords)[2]) const;

  bool delete_tiles_for_real();
};
