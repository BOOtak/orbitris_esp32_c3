#pragma once

#include <cstdint>

#include "game_utils.h"
#include "world.h"

constexpr int PATH_MAX_CELLS = MAZE_ROWS * MAZE_COLS;

struct Path {
  int16_t cells[PATH_MAX_CELLS];
  int count;
};

bool is_wall_cell(int col, int row);

// Fills `out` with the cells after `start` up to and including `goal`.
// Returns false if no path exists. `out.count == 0` when start == goal.
bool find_path(int start_col, int start_row, int goal_col, int goal_row, Path& out);

Vector2 cell_to_world(int col, int row);

void world_to_cell(Vector2 pos, int& col, int& row);
