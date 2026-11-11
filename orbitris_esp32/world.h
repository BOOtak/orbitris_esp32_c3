#pragma once

#include "const.h"
#include "game_utils.h"

constexpr int BLOCK_SIZE = 16;
constexpr int CELL_BLOCKS = 3;
constexpr int MAZE_COLS = 15;
constexpr int MAZE_ROWS = 15;

constexpr int MAZE_BLOCK_COLS = MAZE_COLS * CELL_BLOCKS;
constexpr int MAZE_BLOCK_ROWS = MAZE_ROWS * CELL_BLOCKS;
constexpr int WORLD_WIDTH = MAZE_BLOCK_COLS * BLOCK_SIZE;
constexpr int WORLD_HEIGHT = MAZE_BLOCK_ROWS * BLOCK_SIZE;
constexpr int MAX_CAMERA_X = WORLD_WIDTH - LCD_WIDTH;
constexpr int MAX_CAMERA_Y = WORLD_HEIGHT - LCD_HEIGHT;

// Extra gap added when pushing the player out of geometry, so the next frame
// does not immediately report an overlap again.
constexpr float COLLISION_EPSILON = 0.01f;
// Longest distance covered per collision substep; shorter than a wall block.
constexpr float MAX_MOVE_STEP = 1.0f;
// Overlaps are resolved one at a time; a couple of passes settle corners.
constexpr int MAX_RESOLVE_ITERATIONS = 4;

// '#' is a solid cell, '.' is walkable floor. Each cell is CELL_BLOCKS x
// CELL_BLOCKS 16px blocks, so corridors are 48px wide.
extern const char MAZE[MAZE_ROWS][MAZE_COLS + 1];

bool is_wall_block(int col, int row);

// Describes how far, and in which direction, the player circle has to move to
// stop overlapping a piece of level geometry.
struct Contact {
  bool hit{};
  Vector2 normal{};
  float depth{};
};

Contact circle_contact(Vector2 pos, float radius, Vector2 center, float center_radius);

// Circle contact against an axis-aligned box centered on the origin. `local` is
// the circle center relative to the box center.
Contact box_contact(Vector2 local, float radius, float half_width, float half_height);

void keep_deeper(Contact& deepest, const Contact& contact);

Contact deepest_contact_walls(Vector2 pos, float radius);

bool resolve_penetration_walls(Vector2& pos, float radius);

void move_with_collision(Vector2& pos, Vector2 delta, float radius);

// Walls plus a set of solid circles (e.g. the player or cats).
void move_with_collision_circles(Vector2& pos, Vector2 delta, float radius,
                                 const Vector2* centers, const float* radii, int count);

void draw_filled_circle(int cx, int cy, int radius, int color);

void draw_circle_lines(int cx, int cy, int radius, int color);
