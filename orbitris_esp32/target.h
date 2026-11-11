#pragma once

#include "game_utils.h"

struct PeeTarget {
  Rectangle rect;
  int col;
  int row;
  float fill;       // 0..1 visual fill
  float fill_step;  // fill added per completed pee
  float mess_weight;
};

constexpr int PEE_FRAMES = 120;
