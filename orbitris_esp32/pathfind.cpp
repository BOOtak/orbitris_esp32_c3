#include "pathfind.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

constexpr float CELL_SIZE_PX = static_cast<float>(BLOCK_SIZE * CELL_BLOCKS);

}  // namespace

bool is_wall_cell(int col, int row) {
  if (col < 0 || col >= MAZE_COLS || row < 0 || row >= MAZE_ROWS) {
    return true;
  }
  return MAZE[row][col] == '#';
}

Vector2 cell_to_world(int col, int row) {
  return { (static_cast<float>(col) + 0.5f) * CELL_SIZE_PX,
           (static_cast<float>(row) + 0.5f) * CELL_SIZE_PX };
}

void world_to_cell(Vector2 pos, int& col, int& row) {
  col = static_cast<int>(std::floor(pos.x / CELL_SIZE_PX));
  row = static_cast<int>(std::floor(pos.y / CELL_SIZE_PX));
  col = std::clamp(col, 0, MAZE_COLS - 1);
  row = std::clamp(row, 0, MAZE_ROWS - 1);
}

bool find_path(int start_col, int start_row, int goal_col, int goal_row, Path& out) {
  out.count = 0;
  if (is_wall_cell(start_col, start_row) || is_wall_cell(goal_col, goal_row)) {
    return false;
  }

  const int start = start_row * MAZE_COLS + start_col;
  const int goal = goal_row * MAZE_COLS + goal_col;
  if (start == goal) {
    return true;
  }

  static int16_t queue[PATH_MAX_CELLS];
  static int16_t parent[PATH_MAX_CELLS];
  static uint8_t visited[PATH_MAX_CELLS];

  std::memset(visited, 0, sizeof(visited));
  for (int i = 0; i < PATH_MAX_CELLS; ++i) {
    parent[i] = -1;
  }

  int head = 0;
  int tail = 0;
  queue[tail++] = static_cast<int16_t>(start);
  visited[start] = 1;

  constexpr int DC[4] = { 0, 0, -1, 1 };
  constexpr int DR[4] = { -1, 1, 0, 0 };

  while (head < tail) {
    const int current = queue[head++];
    if (current == goal) {
      break;
    }

    const int col = current % MAZE_COLS;
    const int row = current / MAZE_COLS;

    for (int i = 0; i < 4; ++i) {
      const int next_col = col + DC[i];
      const int next_row = row + DR[i];
      if (is_wall_cell(next_col, next_row)) {
        continue;
      }

      const int next = next_row * MAZE_COLS + next_col;
      if (visited[next]) {
        continue;
      }

      visited[next] = 1;
      parent[next] = static_cast<int16_t>(current);
      queue[tail++] = static_cast<int16_t>(next);
    }
  }

  if (!visited[goal]) {
    return false;
  }

  int length = 0;
  for (int cell = goal; cell != start && cell >= 0; cell = parent[cell]) {
    out.cells[length++] = static_cast<int16_t>(cell);
  }

  for (int i = 0; i < length / 2; ++i) {
    const int16_t tmp = out.cells[i];
    out.cells[i] = out.cells[length - 1 - i];
    out.cells[length - 1 - i] = tmp;
  }

  out.count = length;
  return true;
}
