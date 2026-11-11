#pragma once

#include "game_utils.h"

struct ScreenPoint {
  int x;
  int y;
};

constexpr int LIGHT_RAYS = 512;

int floor_div(int value, int divisor);

int ceil_div(int value, int divisor);

int to_subpixel(float value);

int integer_sqrt(int value);

// Fills `out_vertices[LIGHT_RAYS]` with the screen-space wall-hit polygon of a
// 360-degree light centered on `player_world`.
void build_light_polygon(Vector2 player_world, int camera_x, int camera_y,
                         ScreenPoint* out_vertices);

// True when `point` is inside the torch radius and not blocked by a wall.
bool is_point_lit(Vector2 origin, Vector2 point);

// 4x4 ordered-dither threshold (0..15) for a screen pixel.
int bayer_value(int x, int y);

// Additive light: draws lit pixels white inside the polygon.
void fill_light_polygon(const ScreenPoint* vertices, int count, int origin_x, int origin_y);

// Solid fill (used for shadow hulls).
void fill_solid_polygon(const ScreenPoint* vertices, int count, int color);

// Subtractive light: clears unlit pixels to black inside the polygon and
// everywhere outside it. Lit pixels keep whatever the scene already drew.
void draw_darkness_polygon(const ScreenPoint* vertices, int count, int origin_x, int origin_y);
