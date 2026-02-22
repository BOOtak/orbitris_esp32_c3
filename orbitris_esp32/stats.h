#pragma once

#include "tilemap.h"

struct Stats {
  bool in_game;
  uint8_t active_block_idx;
  uint8_t next_block_idx;
  uint8_t reserved;
  int game_points;
  TileBitmap tilemap;
};
