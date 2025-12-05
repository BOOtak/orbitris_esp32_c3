#pragma once

#include <cstdint>

#include "game_utils.h"

constexpr int BLOCK_SIZE = 4;
constexpr int TILE_W = 8;  // px
constexpr int TILE_H = 8;

struct Tetramino {
  Vector2 center;  // in tiles;
  uint8_t data[4][4][4];
};

struct ActiveTetramino {
  int rot_index;
  Vector2 pos;
  Tetramino* block;
  Vector2 oldPos;  // to slide in place
  Vector2 targetPos;
  float progress;  // [0.0, 1.0]
};

extern Tetramino Z_Block;
extern Tetramino O_Block;
extern Tetramino I_Block;
extern Tetramino L_Block;
extern Tetramino J_Block;
extern Tetramino S_Block;
extern Tetramino T_Block;

extern Tetramino* Blocks[];

Tetramino* get_random_block();

void draw_tile(int x, int y, int size);

void draw_tetramino(const ActiveTetramino& tetramino);
