#include "tilemap.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "const.h"
#include "draw.h"
#include "trace.h"

// Tilemap location and dimensions
constexpr int tileMapPosX = (LCD_WIDTH - TILES_X * TILE_W) / 2;
constexpr int tileMapPosY = (LCD_HEIGHT - TILES_Y * TILE_H) / 2;
constexpr int tileMapWidth = TILES_X * TILE_W;
constexpr int tileMapHeight = TILES_Y * TILE_H;
constexpr Rectangle tileMapRect = { tileMapPosX, tileMapPosY, tileMapWidth, tileMapHeight };

// Update logic constants
constexpr float deleteProgressSpeed = 0.5f;

Tilemap::Tilemap() {
  init();
}

void Tilemap::init() {
  game_points = 0;
  tile_out_of_bounds = false;

  std::memset(tilemap_, 0, sizeof(tilemap_));
  constexpr int filled_rows = 4;
  constexpr int offset = TILES_X / 2 - filled_rows / 2;
  for (size_t x = offset; x < offset + filled_rows; x++) {
    for (size_t y = offset; y < offset + filled_rows; y++) {
      tilemap_[x][y].occupied = true;
    }
  }

  tile_delete_info_ = {};
}

void Tilemap::update() {
  if (tile_delete_info_.draw_size > 0) {
    tile_delete_info_.draw_size -= deleteProgressSpeed;
  } else {
    tile_delete_info_.should_delete = true;
  }

  if (tile_delete_info_.should_delete) {
    delete_tiles_for_real();
    tile_delete_info_.should_delete = false;
  }
}

void Tilemap::draw() const {
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      if (is_blank(tilemap_[i][j])) {
        continue;
      }

      size_t size = TILE_W;
      if (tilemap_[i][j].flags == TileFlags::TO_DELETE) {
        size = std::max((size_t)0, (size_t)tile_delete_info_.draw_size);
      }

      int posX = (i - TILES_X / 2) * TILE_W + CENTER_X + (TILE_W - size) / 2;
      int posY = (j - TILES_Y / 2) * TILE_H + CENTER_Y + (TILE_H - size) / 2;
      draw_tile(posX, posY, size);
    }
  }
}

Rectangle Tilemap::intersect_tiles(const ActiveTetramino& block) {
  // TODO: optimize and check collisions only with tiles surrounding the
  // tilemap. Or don't do that, it should work just fine as-is

  Rectangle blockRect{
    block.pos.x - block.block->center.x * TILE_W,
    block.pos.y - block.block->center.y * TILE_H,
    TILE_W * BLOCK_SIZE,
    TILE_H * BLOCK_SIZE
  };

  if (check_collision_recs(tileMapRect, blockRect)) {
    Rectangle tileRect{ 0.0f, 0.0f, TILE_W, TILE_H };

    for (size_t i = 0; i < TILES_X; i++) {
      for (size_t j = 0; j < TILES_Y; j++) {
        if (is_blank(tilemap_[i][j])) {
          continue;
        }

        int posX = (i - TILES_X / 2) * TILE_W + CENTER_X;
        int posY = (j - TILES_Y / 2) * TILE_H + CENTER_Y;
        tileRect.x = posX;
        tileRect.y = posY;

        if (check_collision_recs(tileRect, blockRect)) {
          // trace("Hit tile %lu %lu\n", i, j);
          int rot_index = block.rot_index;
          uint8_t (*data)[4] = block.block->data[rot_index];
          Rectangle targetRect = { 0, 0, TILE_W, TILE_H };
          for (size_t y = 0; y < BLOCK_SIZE; y++) {
            for (size_t x = 0; x < BLOCK_SIZE; x++) {
              if (data[y][x] == 0) {
                continue;
              }

              targetRect.x = blockRect.x + x * TILE_W;
              targetRect.y = blockRect.y + y * TILE_H;

              if (check_collision_recs(tileRect, targetRect)) {
                // trace("Collision!\n");
                return get_collision_rec(tileRect, targetRect);
              }
            }
          }
        }
      }
    }
  }

  return {};
}

void Tilemap::place_tetramino(const ActiveTetramino& block) {
  int coords[BLOCK_SIZE][2]{};
  get_tetramino_tilemap_pos(block, coords);
  for (size_t i = 0; i < BLOCK_SIZE; i++) {
    // trace("Add %d %d\n", coords[i][0], coords[i][1]);
    tilemap_[coords[i][0]][coords[i][1]].occupied = true;
  }

  check_rows();
  check_bounds();
}

bool Tilemap::can_move(const ActiveTetramino& block, int dx, int dy) const {
  int coords[BLOCK_SIZE][2]{};
  get_tetramino_tilemap_pos(block, coords);
  for (size_t i = 0; i < BLOCK_SIZE; i++) {
    if (!is_blank(tilemap_[coords[i][0] + dx][coords[i][1] + dy])) {
      return false;
    }
  }

  return true;
}

void Tilemap::check_rows() {
  int hits = 0;

  constexpr auto row_start = (TILES_Y - ROW_LENGTH) / 2;
  for (size_t i = 0; i < TILES_X; i++) {
    bool hit = true;
    for (size_t j = row_start; j < row_start + ROW_LENGTH; j++) {
      if (is_blank(tilemap_[i][j])) {
        hit = false;
        break;
      }
    }

    if (hit) {
      hits++;
      trace("Filled by Y at x = %lu\n", i);
      tile_delete_info_.columns[i] = true;
      for (size_t j1 = 0; j1 < TILES_Y; j1++) {
        if (!is_blank(tilemap_[i][j1])) {
          tilemap_[i][j1].flags = TileFlags::TO_DELETE;
        }
      }
    }
  }

  constexpr auto col_start = (TILES_X - ROW_LENGTH) / 2;
  for (size_t j = 0; j < TILES_Y; j++) {
    bool hit = true;
    for (size_t i = col_start; i < col_start + ROW_LENGTH; i++) {
      if (is_blank(tilemap_[i][j])) {
        hit = false;
        break;
      }
    }

    if (hit) {
      hits++;
      trace("Filled by X at y = %lu\n", j);
      tile_delete_info_.rows[j] = true;
      for (size_t i1 = 0; i1 < TILES_X; i1++) {
        if (!is_blank(tilemap_[i1][j])) {
          tilemap_[i1][j].flags = TileFlags::TO_DELETE;
        }
      }
    }
  }

  if (hits > 0) {
    tile_delete_info_.draw_size = TILE_W;
  }

  switch (hits) {
    case 1:
      game_points += 40;
      break;
    case 2:
      game_points += 100;
      break;
    case 3:
      game_points += 300;
      break;
    case 4:
      game_points += 1200;
      break;
    default:
      if (hits > 4) {
        game_points += 2400;
      }
      break;
  }

  trace("Game points: %d\n", game_points);
}

void Tilemap::check_bounds() {
  constexpr int PADDING = (TILES_X - DEATH_LENGTH) / 2;
  const int startX = PADDING, startY = PADDING;
  const int endX = startX + DEATH_LENGTH, endY = startY + DEATH_LENGTH;

  tile_out_of_bounds = false;

  for (size_t i = 0; i < startY; i++) {
    for (size_t j = 0; j < TILES_X; j++) {
      check_and_flag_oob(i, j);
    }
  }

  for (size_t i = endY; i < TILES_Y; i++) {
    for (size_t j = 0; j < TILES_X; j++) {
      check_and_flag_oob(i, j);
    }
  }

  // We only check the columns outside the center for the middle rows
  for (size_t i = startY; i < endY; i++) {
    for (size_t j = 0; j < startX; j++) {
      check_and_flag_oob(i, j);
    }
  }

  // We only check the columns outside the center for the middle rows
  for (size_t i = startY; i < endY; i++) {
    for (size_t j = endX; j < TILES_X; j++) {
      check_and_flag_oob(i, j);
    }
  }
}

void Tilemap::check_and_flag_oob(size_t i, size_t j) {
  if (!is_blank(tilemap_[j][i])) {
    trace("OOB tile at %lu %lu!\n", i, j);
    tilemap_[j][i].flags = TileFlags::OOB;
    tile_out_of_bounds = true;
  }
}

void Tilemap::get_tetramino_tilemap_pos_corner(const ActiveTetramino& block, int& x, int& y) const {
  size_t idx = 0;
  float deltaX = block.pos.x - block.block->center.x * TILE_W - tileMapPosX;
  float deltaY = block.pos.y - block.block->center.y * TILE_H - tileMapPosY;
  int ix = (int)round(deltaX / TILE_W);
  int iy = (int)round(deltaY / TILE_H);

  x = ix;
  y = iy;
}

Vector2 Tilemap::get_tile_pos(int ix, int iy) const {
  Vector2 result{};
  result.x = ix * TILE_W + CENTER_X - (TILE_W * TILES_X / 2);
  result.y = iy * TILE_H + CENTER_Y - (TILE_H * TILES_Y / 2);
  return result;
}

bool Tilemap::is_blank(int ix, int iy) const {
  return is_blank(tilemap_[ix][iy]);
}

void Tilemap::get_tetramino_tilemap_pos(const ActiveTetramino& block, int (*coords)[2] /* int[4][2] */) const {
  size_t idx = 0;
  float deltaX = block.pos.x - block.block->center.x * TILE_W - tileMapPosX;
  float deltaY = block.pos.y - block.block->center.y * TILE_H - tileMapPosY;
  int ix = 0, iy = 0;
  get_tetramino_tilemap_pos_corner(block, ix, iy);

  // TODO: Check bounds!
  uint8_t (*data)[BLOCK_SIZE] = block.block->data[block.rot_index];
  for (size_t y = 0; y < BLOCK_SIZE; y++) {
    for (size_t x = 0; x < BLOCK_SIZE; x++) {
      if (data[y][x] == 0) {
        continue;
      }

      coords[idx][0] = ix + x;
      coords[idx][1] = iy + y;
      idx++;
    }
  }
}

void Tilemap::delete_tiles_for_real() {
  for (size_t i = 0; i < TILES_X; i++) {
    if (tile_delete_info_.columns[i]) {
      size_t di = 1, max_i = TILES_X;
      if (i < TILES_X / 2) {
        trace("Move right\n");
        di = -1;
        max_i = 0;
      } else {
        trace("Move left\n");
      }

      for (size_t i1 = i; i1 != max_i; i1 = i1 + di) {
        if ((di > 0 && i1 == TILES_X - 1) || (di < 0 && i1 == 0)) {
          tile_delete_info_.columns[i1] = false;
        } else {
          tile_delete_info_.columns[i1] = tile_delete_info_.columns[i1 + di];
        }

        for (size_t j = 0; j < TILES_Y; j++) {
          if ((di > 0 && i1 == TILES_X - 1) || (di < 0 && i1 == 0)) {
            tilemap_[i1][j] = {};
          } else {
            tilemap_[i1][j] = tilemap_[i1 + di][j];
          }
        }
      }

      // check same column again
      if (di > 0) {
        i = i - 1;
      }
    }
  }

  for (size_t j = 0; j < TILES_Y; j++) {
    if (tile_delete_info_.rows[j]) {
      size_t dj = 1, max_j = TILES_Y;
      if (j < TILES_Y / 2) {
        trace("Move down\n");
        dj = -1;
        max_j = 0;
      } else {
        trace("Move up\n");
      }

      for (size_t j1 = j; j1 != max_j; j1 = j1 + dj) {
        if ((dj > 0 && j1 == TILES_Y - 1) || (dj < 0 && j1 == 0)) {
          tile_delete_info_.rows[j1] = false;
        } else {
          tile_delete_info_.rows[j1] = tile_delete_info_.rows[j1 + dj];
        }

        for (size_t i = 0; i < TILES_X; i++) {
          if ((dj > 0 && j1 == TILES_Y - 1) || (dj < 0 && j1 == 0)) {
            tilemap_[i][j1] = {};
          } else {
            tilemap_[i][j1] = tilemap_[i][j1 + dj];
          }
        }
      }

      // check same row again
      if (dj > 0) {
        j = j - 1;
      }
    }
  }
}

bool Tilemap::is_blank(const Tile& tile) const {
  return !tile.occupied;
}
