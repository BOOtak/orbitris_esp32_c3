#include "cat.h"

#include <algorithm>
#include <cmath>

#include "game_utils.h"
#include "light.h"
#include "pathfind.h"
#include "target.h"
#include "trace.h"
#include "world.h"

namespace {

constexpr int CAT_SMOOTH_LOOKAHEAD = 12;
constexpr float CAT_SMOOTH_STEP = 4.0f;

enum class PathStatus {
  Moving,
  Arrived,
  Blocked,
};

constexpr char* STATE_NAMES[] = {
  "Wander",
  "SeekTarget",
  "Peeing",
  "Flee",
  "Carried",
};

int random_bladder_frames() {
  return get_random_value(CAT_BLADDER_MIN_FRAMES, CAT_BLADDER_MAX_FRAMES);
}

int cell_col(int cell) {
  return cell % MAZE_COLS;
}

int cell_row(int cell) {
  return cell / MAZE_COLS;
}

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

// String-pulling: keep only the waypoints needed, turning axis-aligned BFS
// paths into long diagonal segments where the maze is open.
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
      const Vector2 to = cell_to_world(cell_col(cell), cell_row(cell));
      if (has_clear_line(from, to, CAT_RADIUS)) {
        best = j;
      }
    }

    const int cell = cat.path.cells[best];
    optimized.cells[write++] = static_cast<int16_t>(cell);
    from = cell_to_world(cell_col(cell), cell_row(cell));
    i = best + 1;
  }

  cat.path = optimized;
  cat.path.count = write;
  cat.waypoint = 0;
}

void start_wander(Cat& cat) {
  cat_set_state(cat, CatState::Wander);
  cat.path.count = 0;
  cat.waypoint = 0;
  cat.blocked_frames = 0;
  cat.rewind_frames = CAT_WANDER_REWIND_FRAMES;
  cat.bladder_frames = random_bladder_frames();
  cat.flee_frames = 0;
  cat.pee_frames = 0;
  cat.target_index = -1;
  cat.goal_col = -1;
  cat.goal_row = -1;
}

void pick_wander_goal(Cat& cat) {
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

bool flee_step_is_toward_player(Vector2 from, Vector2 to, Vector2 player_pos) {
  const float away_x = from.x - player_pos.x;
  const float away_y = from.y - player_pos.y;
  const float step_x = to.x - from.x;
  const float step_y = to.y - from.y;

  const float away_len = std::sqrt(away_x * away_x + away_y * away_y);
  const float step_len = std::sqrt(step_x * step_x + step_y * step_y);
  if (away_len < 0.001f || step_len < 0.001f) {
    return false;
  }

  const float dot = (away_x * step_x + away_y * step_y) / (away_len * step_len);
  return dot < -0.1f;
}

void start_flee(Cat& cat, Vector2 player_pos) {
  cat_set_state(cat, CatState::Flee);
  cat.flee_frames = CAT_FLEE_FRAMES;
  cat.blocked_frames = 0;
  cat.path.count = 0;

  int cat_col = 0;
  int cat_row = 0;
  world_to_cell(cat.pos, cat_col, cat_row);

  const float away_x = cat.pos.x - player_pos.x;
  const float away_y = cat.pos.y - player_pos.y;
  const bool have_away = away_x * away_x + away_y * away_y > 0.001f;

  int best_col = -1;
  int best_row = -1;
  float best_dist_sq = -1.0f;
  Path best_path{};

  for (int attempt = 0; attempt < 24; ++attempt) {
    const int col = get_random_value(0, MAZE_COLS - 1);
    const int row = get_random_value(0, MAZE_ROWS - 1);
    if (is_wall_cell(col, row)) {
      continue;
    }

    const Vector2 center = cell_to_world(col, row);
    if (is_point_lit(player_pos, center)) {
      continue;
    }

    if (have_away) {
      const float toward =
        (center.x - cat.pos.x) * away_x + (center.y - cat.pos.y) * away_y;
      if (toward <= 0.0f) {
        continue;
      }
    }

    const float dx = center.x - player_pos.x;
    const float dy = center.y - player_pos.y;
    const float dist_sq = dx * dx + dy * dy;
    if (dist_sq <= best_dist_sq) {
      continue;
    }

    Path candidate{};
    if (!find_path(cat_col, cat_row, col, row, candidate)) {
      continue;
    }
    if (candidate.count == 0) {
      continue;
    }

    best_col = col;
    best_row = row;
    best_dist_sq = dist_sq;
    best_path = candidate;
  }

  if (best_col < 0) {
    return;
  }

  cat.path = best_path;
  optimize_path(cat);

  if (cat.path.count > 0) {
    const int first_cell = cat.path.cells[0];
    const Vector2 first = cell_to_world(cell_col(first_cell), cell_row(first_cell));
    if (flee_step_is_toward_player(cat.pos, first, player_pos)) {
      cat.path.count = 0;
      cat.waypoint = 0;
      return;
    }
  }

  cat.waypoint = 0;
  cat.goal_col = best_col;
  cat.goal_row = best_row;
}

bool start_seek(Cat& cat, PeeTarget* targets, int target_count, Vector2 player_pos) {
  int col = 0;
  int row = 0;
  world_to_cell(cat.pos, col, row);

  int best_index = -1;
  int best_distance = PATH_MAX_CELLS + 1;
  Path best_path{};

  for (int i = 0; i < target_count; ++i) {
    if (targets[i].fill >= 1.0f) {
      continue;
    }
    if (cat.failed_target_frames > 0 && i == cat.failed_target) {
      continue;
    }

    const Vector2 target_center = cell_to_world(targets[i].col, targets[i].row);
    if (is_point_lit(player_pos, target_center)) {
      continue;
    }

    Path candidate{};
    if (!find_path(col, row, targets[i].col, targets[i].row, candidate)) {
      continue;
    }
    if (candidate.count == 0) {
      continue;
    }

    if (candidate.count < best_distance) {
      best_index = i;
      best_distance = candidate.count;
      best_path = candidate;
    }
  }

  if (best_index < 0) {
    return false;
  }

  cat_set_state(cat, CatState::SeekTarget);
  trace("Target: index %d, x: %.0f, y: %.0f\n", best_index, targets[best_index].rect.x,
        targets[best_index].rect.y);
  cat.target_index = best_index;
  cat.path = best_path;
  optimize_path(cat);
  cat.waypoint = 0;
  cat.blocked_frames = 0;
  cat.goal_col = targets[best_index].col;
  cat.goal_row = targets[best_index].row;
  return true;
}

void start_peeing(Cat& cat, PeeTarget* targets) {
  if (cat.target_index < 0) {
    start_wander(cat);
    return;
  }

  cat_set_state(cat, CatState::Peeing);
  cat.pee_frames = PEE_FRAMES;
  cat.pee_start_fill = targets[cat.target_index].fill;
  cat.failed_target = -1;
  cat.failed_target_frames = 0;
  cat.path.count = 0;
  cat.waypoint = 0;
  cat.blocked_frames = 0;
}

PathStatus move_along_path(Cat& cat, float speed, Vector2 player_pos, float player_radius,
                           Cat* cats, int cat_count, int self_index) {
  if (cat.path.count == 0 || cat.waypoint >= cat.path.count) {
    return PathStatus::Arrived;
  }

  const int cell = cat.path.cells[cat.waypoint];
  const Vector2 target = cell_to_world(cell_col(cell), cell_row(cell));

  const float dx = target.x - cat.pos.x;
  const float dy = target.y - cat.pos.y;
  const float dist = std::sqrt(dx * dx + dy * dy);

  if (dist <= CAT_WAYPOINT_REACH) {
    ++cat.waypoint;
    cat.blocked_frames = 0;
    if (cat.waypoint >= cat.path.count) {
      cat.path.count = 0;
      return PathStatus::Arrived;
    }
    return PathStatus::Moving;
  }

  Vector2 avoid_centers[CAT_MAX_AVOID];
  float avoid_radii[CAT_MAX_AVOID];
  int avoid_count = 0;

  avoid_centers[avoid_count] = player_pos;
  avoid_radii[avoid_count] = player_radius;
  ++avoid_count;

  for (int i = 0; i < cat_count && avoid_count < CAT_MAX_AVOID; ++i) {
    if (i == self_index || cats[i].boxed || cats[i].state == CatState::Carried) {
      continue;
    }
    avoid_centers[avoid_count] = cats[i].pos;
    avoid_radii[avoid_count] = CAT_RADIUS;
    ++avoid_count;
  }

  const Vector2 before = cat.pos;
  const float step = std::min(speed, dist);
  const Vector2 delta{ dx / dist * step, dy / dist * step };
  move_with_collision_circles(cat.pos, delta, CAT_RADIUS, avoid_centers, avoid_radii,
                              avoid_count);

  const float moved_x = cat.pos.x - before.x;
  const float moved_y = cat.pos.y - before.y;
  const float moved_sq = moved_x * moved_x + moved_y * moved_y;
  const float min_moved = speed * 0.5f;

  if (moved_sq < min_moved * min_moved) {
    ++cat.blocked_frames;
    if (cat.blocked_frames >= CAT_BLOCKED_REPATH_FRAMES) {
      cat.blocked_frames = 0;
      return PathStatus::Blocked;
    }
  } else {
    cat.blocked_frames = 0;
  }

  return PathStatus::Moving;
}

void move_away_from_player(Cat& cat, Vector2 player_pos, float player_radius, Cat* cats,
                           int cat_count, int self_index) {
  float dx = cat.pos.x - player_pos.x;
  float dy = cat.pos.y - player_pos.y;
  const float dist = std::sqrt(dx * dx + dy * dy);

  Vector2 dir{ 0.0f, -1.0f };
  if (dist > 0.001f) {
    dir = { dx / dist, dy / dist };
  }

  Vector2 avoid_centers[CAT_MAX_AVOID];
  float avoid_radii[CAT_MAX_AVOID];
  int avoid_count = 0;

  avoid_centers[avoid_count] = player_pos;
  avoid_radii[avoid_count] = player_radius;
  ++avoid_count;

  for (int i = 0; i < cat_count && avoid_count < CAT_MAX_AVOID; ++i) {
    if (i == self_index || cats[i].boxed || cats[i].state == CatState::Carried) {
      continue;
    }
    avoid_centers[avoid_count] = cats[i].pos;
    avoid_radii[avoid_count] = CAT_RADIUS;
    ++avoid_count;
  }

  const Vector2 delta{ dir.x * CAT_FLEE_SPEED, dir.y * CAT_FLEE_SPEED };
  move_with_collision_circles(cat.pos, delta, CAT_RADIUS, avoid_centers, avoid_radii,
                              avoid_count);
}

}  // namespace

void cat_init(Cat& cat, int col, int row) {
  cat.pos = cell_to_world(col, row);
  cat_set_state(cat, CatState::Wander);
  cat.path.count = 0;
  cat.waypoint = 0;
  cat.rewind_frames = CAT_WANDER_REWIND_FRAMES;
  cat.blocked_frames = 0;
  cat.bladder_frames = random_bladder_frames();
  cat.flee_frames = 0;
  cat.pee_frames = 0;
  cat.target_index = -1;
  cat.failed_target = -1;
  cat.failed_target_frames = 0;
  cat.pee_start_fill = 0.0f;
  cat.goal_col = -1;
  cat.goal_row = -1;
  cat.boxed = false;
}

void cat_set_state(Cat& cat, CatState state) {
  if (cat.state == state) {
    return;
  }
  trace("State %s -> %s\n", STATE_NAMES[static_cast<size_t>(cat.state)],
        STATE_NAMES[static_cast<size_t>(state)]);
  cat.state = state;
}

void cat_carry(Cat& cat) {
  cat_set_state(cat, CatState::Carried);
  cat.path.count = 0;
  cat.waypoint = 0;
  cat.blocked_frames = 0;
  cat.flee_frames = 0;
  cat.pee_frames = 0;
  cat.target_index = -1;
  cat.failed_target = -1;
  cat.failed_target_frames = 0;
}

void cat_drop(Cat& cat, Vector2 player_pos) {
  cat.pos = player_pos;
  cat_set_state(cat, CatState::Flee);
  cat.flee_frames = CAT_FLEE_FRAMES;
  cat.path.count = 0;
  cat.waypoint = 0;
  cat.blocked_frames = 0;
  cat.target_index = -1;
  cat.failed_target = -1;
  cat.failed_target_frames = 0;
}

void cat_update(Cat& cat, int self_index, Cat* cats, int cat_count, Vector2 player_pos,
                float player_radius, PeeTarget* targets, int target_count, float* mess) {
  if (cat.boxed || cat.state == CatState::Carried) {
    return;
  }

  if (cat.failed_target_frames > 0) {
    --cat.failed_target_frames;
    if (cat.failed_target_frames == 0) {
      cat.failed_target = -1;
    }
  }

  if (cat.state == CatState::Peeing) {
    if (cat.target_index < 0 || cat.target_index >= target_count) {
      start_wander(cat);
      return;
    }

    PeeTarget& target = targets[cat.target_index];

    if (is_point_lit(player_pos, cat.pos)) {
      target.fill = cat.pee_start_fill;
      cat.failed_target = cat.target_index;
      cat.failed_target_frames = CAT_FAILED_TARGET_FRAMES;
      start_flee(cat, player_pos);
      if (cat.path.count == 0) {
        start_wander(cat);
      }
      return;
    }

    if (cat.pee_frames > 0) {
      --cat.pee_frames;
    }

    const float progress = 1.0f - static_cast<float>(cat.pee_frames) / PEE_FRAMES;
    target.fill = std::min(1.0f, cat.pee_start_fill + progress * target.fill_step);

    if (cat.pee_frames <= 0) {
      target.fill = std::min(1.0f, cat.pee_start_fill + target.fill_step);
      *mess = std::min(100.0f, *mess + target.mess_weight);
      start_wander(cat);
    }
    return;
  }

  const bool lit = is_point_lit(player_pos, cat.pos);
  if (lit && cat.state != CatState::Flee) {
    if (cat.state == CatState::SeekTarget) {
      cat.failed_target = cat.target_index;
      cat.failed_target_frames = CAT_FAILED_TARGET_FRAMES;
    }
    start_flee(cat, player_pos);
    if (cat.path.count == 0) {
      start_wander(cat);
    }
    return;
  }

  switch (cat.state) {
    case CatState::Wander:
      {
        if (cat.bladder_frames > 0) {
          --cat.bladder_frames;
        }
        if (cat.bladder_frames == 0) {
          if (start_seek(cat, targets, target_count, player_pos)) {
            return;
          }
          cat.bladder_frames = random_bladder_frames();
        }

        if (cat.rewind_frames > 0) {
          --cat.rewind_frames;
        }
        if (cat.path.count == 0 || cat.waypoint >= cat.path.count || cat.rewind_frames == 0) {
          pick_wander_goal(cat);
        }
        if (cat.path.count == 0) {
          return;
        }

        const PathStatus status =
          move_along_path(cat, CAT_SPEED, player_pos, player_radius, cats, cat_count, self_index);
        if (status == PathStatus::Arrived || status == PathStatus::Blocked) {
          pick_wander_goal(cat);
        }
        break;
      }

    case CatState::SeekTarget:
      {
        if (cat.target_index < 0 || cat.target_index >= target_count || targets[cat.target_index].fill >= 1.0f) {
          if (!start_seek(cat, targets, target_count, player_pos)) {
            start_wander(cat);
          }
          return;
        }

        const PathStatus status =
          move_along_path(cat, CAT_SPEED, player_pos, player_radius, cats, cat_count, self_index);
        if (status == PathStatus::Arrived) {
          start_peeing(cat, targets);
        } else if (status == PathStatus::Blocked) {
          int col = 0;
          int row = 0;
          world_to_cell(cat.pos, col, row);
          const PeeTarget& target = targets[cat.target_index];
          if (find_path(col, row, target.col, target.row, cat.path) && cat.path.count > 0) {
            optimize_path(cat);
            cat.waypoint = 0;
          } else if (!start_seek(cat, targets, target_count, player_pos)) {
            start_wander(cat);
          }
        }
        break;
      }

    case CatState::Flee:
      {
        if (cat.flee_frames > 0) {
          --cat.flee_frames;
        }
        if (cat.flee_frames == 0) {
          start_wander(cat);
          return;
        }

        if (cat.path.count == 0) {
          move_away_from_player(cat, player_pos, player_radius, cats, cat_count, self_index);
          return;
        }

        if (cat.waypoint < cat.path.count) {
          const int cell = cat.path.cells[cat.waypoint];
          const Vector2 target = cell_to_world(cell_col(cell), cell_row(cell));
          if (flee_step_is_toward_player(cat.pos, target, player_pos)) {
            cat.path.count = 0;
            move_away_from_player(cat, player_pos, player_radius, cats, cat_count, self_index);
            return;
          }
        }

        const PathStatus status =
          move_along_path(cat, CAT_FLEE_SPEED, player_pos, player_radius, cats, cat_count,
                          self_index);
        if (status == PathStatus::Arrived) {
          start_wander(cat);
        } else if (status == PathStatus::Blocked) {
          cat.path.count = 0;
        }
        break;
      }

    case CatState::Peeing:
      break;
  }
}
