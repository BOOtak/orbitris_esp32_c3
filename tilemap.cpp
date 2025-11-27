#include "tilemap.h"

#include <Arduino.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "draw.h"
#include "sharp_display.h"

// Tilemap location and dimensions
constexpr int centerX = LCD_WIDTH / 2;
constexpr int centerY = LCD_HEIGHT / 2;
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

  std::memset(tilemap_, 0, sizeof(tilemap_));
  tilemap_[6][TILES_Y / 2 - 1].occupied = true;
  tilemap_[6][TILES_Y / 2].occupied = true;
  tilemap_[7][TILES_Y / 2 - 1].occupied = true;
  tilemap_[7][TILES_Y / 2].occupied = true;
  tilemap_[8][TILES_Y / 2 - 1].occupied = true;
  tilemap_[8][TILES_Y / 2].occupied = true;
  tilemap_[9][TILES_Y / 2 - 1].occupied = true;
  tilemap_[9][TILES_Y / 2].occupied = true;
  tilemap_[10][TILES_Y / 2 - 1].occupied = true;
  tilemap_[10][TILES_Y / 2].occupied = true;
  tilemap_[11][TILES_Y / 2 - 1].occupied = true;
  tilemap_[11][TILES_Y / 2].occupied = true;
  tilemap_[12][TILES_Y / 2 - 1].occupied = true;
  tilemap_[12][TILES_Y / 2].occupied = true;


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

void Tilemap::draw() {
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      if (is_blank(tilemap_[i][j])) {
        continue;
      }

      size_t size = TILE_W;
      if (tilemap_[i][j].flags == TileFlags::TO_DELETE) {
        size = max((size_t)0, (size_t)tile_delete_info_.draw_size);
      }

      int posX = (i - TILES_X / 2) * TILE_W + centerX + (TILE_W - size) / 2;
      int posY = (j - TILES_Y / 2) * TILE_H + centerY + (TILE_H - size) / 2;
      draw_rectangle_lines(posX, posY, size, size, 0);
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

        int posX = (i - TILES_X / 2) * TILE_W + centerX;
        int posY = (j - TILES_Y / 2) * TILE_H + centerY;
        tileRect.x = posX;
        tileRect.y = posY;

        if (check_collision_recs(tileRect, blockRect)) {
          // Serial.printf("Hit tile %lu %lu\n", i, j);
          int rot_index = block.rot_index;
          uint8_t(*data)[4] = block.block->data[rot_index];
          Rectangle targetRect = { .x = 0, .y = 0, .width = TILE_W, .height = TILE_H };
          for (size_t y = 0; y < BLOCK_SIZE; y++) {
            for (size_t x = 0; x < BLOCK_SIZE; x++) {
              if (data[y][x] == 0) {
                continue;
              }

              targetRect.x = blockRect.x + x * TILE_W;
              targetRect.y = blockRect.y + y * TILE_H;

              if (check_collision_recs(tileRect, targetRect)) {
                // Serial.printf("Collision!\n");
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
    // Serial.printf("Add %d %d\n", coords[i][0], coords[i][1]);
    // tilemap[coords[i][0]][coords[i][1]] = block.block->color;
    tilemap_[coords[i][0]][coords[i][1]].occupied = true;
  }

  check_rows();
  // CheckTilesOOB();
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
      Serial.printf("Filled by Y at x = %lu\n", i);
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
      Serial.printf("Filled by X at y = %lu\n", j);
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

  Serial.printf("Game points: %d\n", game_points);
}

void Tilemap::get_tetramino_tilemap_pos_corner(const ActiveTetramino& block, int* x, int* y) {
  size_t idx = 0;
  float deltaX = block.pos.x - block.block->center.x * TILE_W - tileMapPosX;
  float deltaY = block.pos.y - block.block->center.y * TILE_H - tileMapPosY;
  int ix = (int)round(deltaX / TILE_W);
  int iy = (int)round(deltaY / TILE_H);

  *x = ix;
  *y = iy;
}

void Tilemap::get_tetramino_tilemap_pos(const ActiveTetramino& block, int (*coords)[2] /* int[4][2] */) {
  size_t idx = 0;
  float deltaX = block.pos.x - block.block->center.x * TILE_W - tileMapPosX;
  float deltaY = block.pos.y - block.block->center.y * TILE_H - tileMapPosY;
  int ix = 0, iy = 0;
  get_tetramino_tilemap_pos_corner(block, &ix, &iy);

  // TODO: Check bounds!
  uint8_t(*data)[BLOCK_SIZE] = block.block->data[block.rot_index];
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
        Serial.printf("Move right\n");
        di = -1;
        max_i = 0;
      } else {
        Serial.printf("Move left\n");
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
        Serial.printf("Move down\n");
        dj = -1;
        max_j = 0;
      } else {
        Serial.printf("Move up\n");
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

bool Tilemap::is_blank(const Tile& tile) {
  return !tile.occupied;
}
