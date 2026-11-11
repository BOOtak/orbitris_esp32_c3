#include "cat.h"

#include <algorithm>
#include <cmath>

#include "game_utils.h"
#include "pathfind.h"
#include "world.h"

namespace {

constexpr int CAT_SMOOTH_LOOKAHEAD = 12;
constexpr float CAT_SMOOTH_STEP = 4.0f;

bool has_clear_line(Vector2 from, Vector2 to, float radius) {
  const float dx = to.x - from.x;
  const float dy = to.y - from.y;
  const float dist = std::sqrt(dx * dx + dy * dy);
  if (dist <= 0.0f) {
    return true;
  }

  const int steps = std::max(1, static_cast<int>(std::ceil(dist / CAT_SMOOTH_STEP)));
  for (int i = 1; i < steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const Vector2 pos{ from.x + dx * t, from.y + dy * t };
    if (deepest_contact_walls(pos, radius).hit) {
      return false;
    }
  }
  return true;
}

// String-pulling: keep only waypoints that are needed. This turns the
// axis-aligned BFS path into long diagonal segments where the maze is open.
void optimize_path(Cat& cat) {
  if (cat.path.count < 2) {
    cat.waypoint = 0;
    return;
  }

  Path optimized{};
  int write = 0;
  Vector2 from = cat.pos;
  int i = 0;

  while (i < cat.path.count) {
    int best = i;
    const int limit = std::min(cat.path.count, i + CAT_SMOOTH_LOOKAHEAD);
    for (int j = i + 1; j < limit; ++j) {
      const int cell = cat.path.cells[j];
      const Vector2 to = cell_to_world(cell % MAZE_COLS, cell / MAZE_COLS);
      if (has_clear_line(from, to, CAT_RADIUS)) {
        best = j;
      }
    }

    const int cell = cat.path.cells[best];
    optimized.cells[write++] = static_cast<int16_t>(cell);
    from = cell_to_world(cell % MAZE_COLS, cell / MAZE_COLS);
    i = best + 1;
  }

  cat.path = optimized;
  cat.path.count = write;
  cat.waypoint = 0;
}

void pick_new_goal(Cat& cat) {
  cat.blocked_frames = 0;
  cat.path.count = 0;

  int col = 0;
  int row = 0;
  world_to_cell(cat.pos, col, row);

  for (int attempt = 0; attempt < 32; ++attempt) {
    const int goal_col = get_random_value(0, MAZE_COLS - 1);
    const int goal_row = get_random_value(0, MAZE_ROWS - 1);
    if (is_wall_cell(goal_col, goal_row)) {
      continue;
    }
    if (goal_col == col && goal_row == row) {
      continue;
    }
    if (!find_path(col, row, goal_col, goal_row, cat.path) || cat.path.count == 0) {
      continue;
    }

    optimize_path(cat);
    cat.waypoint = 0;
    cat.goal_col = goal_col;
    cat.goal_row = goal_row;
    cat.rewind_frames = CAT_WANDER_REWIND_FRAMES;
    cat.blocked_frames = 0;
    return;
  }

  cat.path.count = 0;
  cat.rewind_frames = 0;
}

}  // namespace

void cat_init(Cat& cat, int col, int row) {
  cat.pos = cell_to_world(col, row);
  cat.state = CatState::Wander;
  cat.path.count = 0;
  cat.waypoint = 0;
  cat.rewind_frames = CAT_WANDER_REWIND_FRAMES;
  cat.blocked_frames = 0;
  cat.goal_col = -1;
  cat.goal_row = -1;
}

void cat_update(Cat& cat, int self_index, Cat* cats, int cat_count, Vector2 player_pos,
                float player_radius) {
  if (cat.rewind_frames > 0) {
    --cat.rewind_frames;
  }

  if (cat.path.count == 0 || cat.waypoint >= cat.path.count || cat.rewind_frames == 0) {
    pick_new_goal(cat);
  }

  if (cat.path.count == 0 || cat.waypoint >= cat.path.count) {
    return;
  }

  const int cell = cat.path.cells[cat.waypoint];
  const int col = cell % MAZE_COLS;
  const int row = cell / MAZE_COLS;
  const Vector2 target = cell_to_world(col, row);

  const float dx = target.x - cat.pos.x;
  const float dy = target.y - cat.pos.y;
  const float dist = std::sqrt(dx * dx + dy * dy);

  if (dist <= CAT_WAYPOINT_REACH) {
    ++cat.waypoint;
    cat.blocked_frames = 0;
    if (cat.waypoint >= cat.path.count) {
      cat.path.count = 0;
      cat.rewind_frames = CAT_WANDER_REWIND_FRAMES;
    }
    return;
  }

  Vector2 avoid_centers[CAT_MAX_AVOID];
  float avoid_radii[CAT_MAX_AVOID];
  int avoid_count = 0;

  avoid_centers[avoid_count] = player_pos;
  avoid_radii[avoid_count] = player_radius;
  ++avoid_count;

  for (int i = 0; i < cat_count && avoid_count < CAT_MAX_AVOID; ++i) {
    if (i == self_index) {
      continue;
    }
    avoid_centers[avoid_count] = cats[i].pos;
    avoid_radii[avoid_count] = CAT_RADIUS;
    ++avoid_count;
  }

  const Vector2 before = cat.pos;
  const float step = std::min(CAT_SPEED, dist);
  const Vector2 delta{ dx / dist * step, dy / dist * step };
  move_with_collision_circles(cat.pos, delta, CAT_RADIUS, avoid_centers, avoid_radii, avoid_count);

  const float moved_x = cat.pos.x - before.x;
  const float moved_y = cat.pos.y - before.y;
  const float moved_sq = moved_x * moved_x + moved_y * moved_y;
  const float min_moved = CAT_SPEED * 0.5f;

  if (moved_sq < min_moved * min_moved) {
    ++cat.blocked_frames;
    if (cat.blocked_frames >= CAT_BLOCKED_REPATH_FRAMES) {
      pick_new_goal(cat);
    }
  } else {
    cat.blocked_frames = 0;
  }
}
