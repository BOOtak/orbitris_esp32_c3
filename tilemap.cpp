#include "tilemap.h"

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


Tilemap::Tilemap() {
  init();
}

void Tilemap::init() {
  std::memset(tilemap_, 0, sizeof(tilemap_));

  tilemap_[TILES_X / 2 - 1][TILES_Y / 2 - 1].occupied = true;
  tilemap_[TILES_X / 2 - 1][TILES_Y / 2].occupied = true;
  tilemap_[TILES_X / 2][TILES_Y / 2 - 1].occupied = true;
  tilemap_[TILES_X / 2][TILES_Y / 2].occupied = true;
}

void Tilemap::draw() {
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      if (is_blank(tilemap_[i][j])) {
        continue;
      }

      int posX = (i - TILES_X / 2) * TILE_W + LCD_WIDTH / 2;
      int posY = (j - TILES_Y / 2) * TILE_H + LCD_HEIGHT / 2;
      draw_rectangle_lines(posX, posY, TILE_W, TILE_H, 0);
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

  // CheckRows();
  // CheckTilesOOB();
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

bool Tilemap::is_blank(const Tile& tile) {
  return !tile.occupied;
}
