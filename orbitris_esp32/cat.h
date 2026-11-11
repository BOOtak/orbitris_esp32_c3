#pragma once

#include "game_utils.h"
#include "pathfind.h"

enum class CatState {
  Wander,
};

constexpr float CAT_RADIUS = 10.0f;
constexpr float CAT_SPEED = 1.3f;
constexpr int CAT_WANDER_REWIND_FRAMES = 240;
constexpr int CAT_BLOCKED_REPATH_FRAMES = 8;
constexpr float CAT_WAYPOINT_REACH = 2.0f;
constexpr int CAT_MAX_AVOID = 8;

struct Cat {
  Vector2 pos{};
  CatState state{ CatState::Wander };
  Path path{};
  int waypoint{};
  int rewind_frames{};
  int blocked_frames{};
  int goal_col{ -1 };
  int goal_row{ -1 };
};

void cat_init(Cat& cat, int col, int row);

void cat_update(Cat& cat, int self_index, Cat* cats, int cat_count, Vector2 player_pos,
                float player_radius);
