#include "demo_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <utility>

#include "const.h"
#include "draw.h"
#include "input.h"
#include "lcd.h"
#include "trace.h"

// Set to 1 to print per-section frame timings (Serial on the device, stdout on
// the desktop). Has no cost when disabled.
#define DEMO_PROFILE 1

namespace {

constexpr int BLOCK_SIZE = 16;
constexpr int CELL_BLOCKS = 3;
constexpr int MAZE_COLS = 20;
constexpr int MAZE_ROWS = 15;

constexpr int MAZE_BLOCK_COLS = MAZE_COLS * CELL_BLOCKS;
constexpr int MAZE_BLOCK_ROWS = MAZE_ROWS * CELL_BLOCKS;
constexpr int WORLD_WIDTH = MAZE_BLOCK_COLS * BLOCK_SIZE;
constexpr int WORLD_HEIGHT = MAZE_BLOCK_ROWS * BLOCK_SIZE;
constexpr int MAX_CAMERA_X = WORLD_WIDTH - LCD_WIDTH;
constexpr int MAX_CAMERA_Y = WORLD_HEIGHT - LCD_HEIGHT;

// '#' is a solid cell, '.' is walkable floor. Each cell is CELL_BLOCKS x
// CELL_BLOCKS 16px blocks, so corridors are 48px wide.
constexpr char MAZE[MAZE_ROWS][MAZE_COLS + 1] = {
  "####################",
  "#........#.........#",
  "#........#.........#",
  "#..####..#..####...#",
  "#..#.....#.....#...#",
  "#..#..####.....#...#",
  "#..#...........#...#",
  "#..####..####..#...#",
  "#........#.....#...#",
  "#..####..#..####...#",
  "#........#.........#",
  "#..#########.......#",
  "#........#.........#",
  "#........#.........#",
  "####################",
};

constexpr float PI = 3.14159265358979323846f;
constexpr float PLAYER_RADIUS = 16.0f;
constexpr float PLAYER_COLLISION_RADIUS = PLAYER_RADIUS;
constexpr Vector2 PLAYER_START{ 120.0f, 120.0f };

constexpr float MOVE_SPEED = 1.5f;
constexpr float RUN_MULTIPLIER = 1.8f;

// Extra gap added when pushing the player out of geometry, so the next frame
// does not immediately report an overlap again.
constexpr float COLLISION_EPSILON = 0.01f;
// Longest distance covered per collision substep; shorter than a wall block.
constexpr float MAX_MOVE_STEP = 1.0f;
// Overlaps are resolved one at a time; a couple of passes settle corners.
constexpr int MAX_RESOLVE_ITERATIONS = 4;

struct CircleObstacle {
  Vector2 center;
  float radius;
};

struct RotatedRectObstacle {
  Vector2 center;
  float half_width;
  float half_height;
  float angle;
};

constexpr CircleObstacle CIRCLE_OBSTACLES[] = {
  { { 300.0f, 96.0f }, 14.0f },
  { { 80.0f, 216.0f }, 11.0f },
};

constexpr RotatedRectObstacle RECT_OBSTACLES[] = {
  { { 280.0f, 216.0f }, 22.0f, 10.0f, PI / 6.0f },
};

constexpr float TWO_PI = 2.0f * PI;
constexpr int LIGHT_RAYS = 512;
constexpr float MAX_LIGHT_DISTANCE = 200.0f;
constexpr float LIGHT_FULL_DISTANCE = 64.0f;
constexpr float SHADOW_LENGTH_FACTOR = 0.35f;
constexpr float SHADOW_MAX_OFFSET = 60.0f;
constexpr float SHADOW_SCALE_PER_OFFSET = 0.012f;
constexpr int SHADOW_CIRCLE_SEGMENTS = 24;
constexpr int MAX_SHADOW_HULL_POINTS = 64;
constexpr float MAX_LIGHT_DISTANCE_SQ = MAX_LIGHT_DISTANCE * MAX_LIGHT_DISTANCE;
constexpr float LIGHT_FULL_DISTANCE_SQ = LIGHT_FULL_DISTANCE * LIGHT_FULL_DISTANCE;

constexpr int BAYER_4X4[4][4] = {
  { 0, 8, 2, 10 },
  { 12, 4, 14, 6 },
  { 3, 11, 1, 9 },
  { 15, 7, 13, 5 },
};

// The C3 has no FPU and the dithered light touches most of the screen every
// frame, so the light math runs in 6.6 fixed point (1/64 px) instead of floats.
constexpr int SUBPIXEL_BITS = 6;
constexpr int SUBPIXEL = 1 << SUBPIXEL_BITS;
constexpr int MAX_SPAN_CROSSINGS = 128;

// Ray casting: positions in 4.4 fixed point (1/16 px), directions in 4.12
// (4096 = 1.0), so the parametric distance fits int32 without 64-bit math.
constexpr int RAY_SUBPIXEL_BITS = 4;
constexpr int RAY_SUBPIXEL = 1 << RAY_SUBPIXEL_BITS;
constexpr int DIR_ONE = 1 << 12;
constexpr int RAY_T_INFINITY = 1 << 24;
constexpr int MAX_LIGHT_DISTANCE_FIXED = static_cast<int>(MAX_LIGHT_DISTANCE) * RAY_SUBPIXEL;

constexpr int dither_cutoff_fixed(int bayer) {
  const float threshold = (static_cast<float>(bayer) + 0.5f) / 16.0f;
  const float cutoff =
    LIGHT_FULL_DISTANCE_SQ +
    (1.0f - threshold) * (MAX_LIGHT_DISTANCE_SQ - LIGHT_FULL_DISTANCE_SQ);
  return static_cast<int>(cutoff * static_cast<float>(SUBPIXEL * SUBPIXEL));
}

// Squared light cutoff per Bayer cell, scaled by SUBPIXEL^2 to match the
// fixed-point distance.
constexpr int DITHER_CUTOFF_FIXED[16] = {
  dither_cutoff_fixed(0),  dither_cutoff_fixed(1),  dither_cutoff_fixed(2),
  dither_cutoff_fixed(3),  dither_cutoff_fixed(4),  dither_cutoff_fixed(5),
  dither_cutoff_fixed(6),  dither_cutoff_fixed(7),  dither_cutoff_fixed(8),
  dither_cutoff_fixed(9),  dither_cutoff_fixed(10), dither_cutoff_fixed(11),
  dither_cutoff_fixed(12), dither_cutoff_fixed(13), dither_cutoff_fixed(14),
  dither_cutoff_fixed(15),
};

struct ScreenPoint {
  int x;
  int y;
};

int floor_div(int value, int divisor) {
  return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
}

int ceil_div(int value, int divisor) {
  return -floor_div(-value, divisor);
}

int to_subpixel(float value) {
  return static_cast<int>(value * SUBPIXEL + (value < 0.0f ? -0.5f : 0.5f));
}

int to_ray_subpixel(float value) {
  return static_cast<int>(value * RAY_SUBPIXEL + (value < 0.0f ? -0.5f : 0.5f));
}

// Largest integer whose square is <= value; used to find the lit half-width.
int integer_sqrt(int value) {
  int result = 0;
  int bit = 1 << 30;
  while (bit > value) {
    bit >>= 2;
  }
  while (bit != 0) {
    if (value >= result + bit) {
      value -= result + bit;
      result = (result >> 1) + bit;
    } else {
      result >>= 1;
    }
    bit >>= 2;
  }
  return result;
}

bool is_wall_block(int col, int row) {
  if (col < 0 || col >= MAZE_BLOCK_COLS || row < 0 || row >= MAZE_BLOCK_ROWS) {
    return true;
  }
  return MAZE[row / CELL_BLOCKS][col / CELL_BLOCKS] == '#';
}

#if DEMO_PROFILE
uint32_t profile_mark_us;
uint32_t profile_totals[3];
uint32_t profile_frames;

void profile_begin() {
  profile_mark_us = platform_micros();
}

void profile_mark(int slot) {
  const uint32_t now = platform_micros();
  profile_totals[slot] += now - profile_mark_us;
  profile_mark_us = now;
}

void profile_end() {
  profile_mark(2);
  ++profile_frames;
  if (profile_frames < 60) {
    return;
  }

  const unsigned light_us = profile_totals[0] / profile_frames;
  const unsigned obstacles_us = profile_totals[1] / profile_frames;
  const unsigned player_us = profile_totals[2] / profile_frames;
  trace("demo: light %u us, obstacles %u us, player %u us, total %u us\n", light_us,
        obstacles_us, player_us, light_us + obstacles_us + player_us);

  profile_totals[0] = 0;
  profile_totals[1] = 0;
  profile_totals[2] = 0;
  profile_frames = 0;
}
#else
inline void profile_begin() {
}

inline void profile_mark(int) {
}

inline void profile_end() {
}
#endif

struct RayDir {
  int x;
  int y;
};

struct ShadowDirection {
  float x;
  float y;
};

// Ray and shadow angles are constant, so all trig is done once instead of
// ~1000 sin/cos calls per frame.
RayDir RAY_DIRECTIONS[LIGHT_RAYS];
ShadowDirection SHADOW_DIRECTIONS[SHADOW_CIRCLE_SEGMENTS];
bool light_tables_ready = false;

void init_light_tables() {
  for (int i = 0; i < LIGHT_RAYS; ++i) {
    const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(LIGHT_RAYS);
    const float dir_x = std::sin(angle);
    const float dir_y = -std::cos(angle);
    RAY_DIRECTIONS[i].x = static_cast<int>(dir_x * DIR_ONE + (dir_x < 0.0f ? -0.5f : 0.5f));
    RAY_DIRECTIONS[i].y = static_cast<int>(dir_y * DIR_ONE + (dir_y < 0.0f ? -0.5f : 0.5f));
  }

  for (int i = 0; i < SHADOW_CIRCLE_SEGMENTS; ++i) {
    const float angle =
      TWO_PI * static_cast<float>(i) / static_cast<float>(SHADOW_CIRCLE_SEGMENTS);
    SHADOW_DIRECTIONS[i] = { std::cos(angle), std::sin(angle) };
  }

  light_tables_ready = true;
}

// Distance to the first wall along the ray, in 4.4 fixed point. The DDA is
// integer: soft-float compares and divisions dominated the frame on the C3.
int raycast_walls(int origin_x, int origin_y, RayDir dir, int max_dist) {
  int col = (origin_x >> RAY_SUBPIXEL_BITS) / BLOCK_SIZE;
  int row = (origin_y >> RAY_SUBPIXEL_BITS) / BLOCK_SIZE;
  if (is_wall_block(col, row)) {
    return 0;
  }

  const int step_x = (dir.x > 0) - (dir.x < 0);
  const int step_y = (dir.y > 0) - (dir.y < 0);
  const int t_delta_x =
    step_x != 0 ? (BLOCK_SIZE * RAY_SUBPIXEL * DIR_ONE) / std::abs(dir.x) : RAY_T_INFINITY;
  const int t_delta_y =
    step_y != 0 ? (BLOCK_SIZE * RAY_SUBPIXEL * DIR_ONE) / std::abs(dir.y) : RAY_T_INFINITY;

  int t_max_x = RAY_T_INFINITY;
  if (step_x > 0) {
    t_max_x = (((col + 1) * BLOCK_SIZE * RAY_SUBPIXEL) - origin_x) * DIR_ONE / dir.x;
  } else if (step_x < 0) {
    t_max_x = ((col * BLOCK_SIZE * RAY_SUBPIXEL) - origin_x) * DIR_ONE / dir.x;
  }

  int t_max_y = RAY_T_INFINITY;
  if (step_y > 0) {
    t_max_y = (((row + 1) * BLOCK_SIZE * RAY_SUBPIXEL) - origin_y) * DIR_ONE / dir.y;
  } else if (step_y < 0) {
    t_max_y = ((row * BLOCK_SIZE * RAY_SUBPIXEL) - origin_y) * DIR_ONE / dir.y;
  }

  while (true) {
    int t;
    if (t_max_x < t_max_y) {
      t = t_max_x;
      col += step_x;
      t_max_x += t_delta_x;
    } else if (t_max_y < t_max_x) {
      t = t_max_y;
      row += step_y;
      t_max_y += t_delta_y;
    } else {
      t = t_max_x;
      col += step_x;
      row += step_y;
      t_max_x += t_delta_x;
      t_max_y += t_delta_y;
    }

    if (t > max_dist) {
      return max_dist;
    }
    if (is_wall_block(col, row)) {
      return t;
    }
  }
}

// Describes how far, and in which direction, the player circle has to move to
// stop overlapping a piece of level geometry.
struct Contact {
  bool hit{};
  Vector2 normal{};
  float depth{};
};

Contact circle_contact(Vector2 pos, float radius, Vector2 center, float center_radius) {
  Contact contact{};

  const float dx = pos.x - center.x;
  const float dy = pos.y - center.y;
  const float min_dist = radius + center_radius;
  const float dist_sq = dx * dx + dy * dy;
  if (dist_sq >= min_dist * min_dist) {
    return contact;
  }

  const float dist = std::sqrt(dist_sq);
  contact.hit = true;
  contact.depth = min_dist - dist;
  contact.normal = dist > 0.0001f ? Vector2{ dx / dist, dy / dist } : Vector2{ 0.0f, -1.0f };
  return contact;
}

// Circle contact against an axis-aligned box centered on the origin. `local` is
// the circle center relative to the box center.
Contact box_contact(Vector2 local, float radius, float half_width, float half_height) {
  Contact contact{};

  const float closest_x = std::clamp(local.x, -half_width, half_width);
  const float closest_y = std::clamp(local.y, -half_height, half_height);
  const float dx = local.x - closest_x;
  const float dy = local.y - closest_y;
  const float dist_sq = dx * dx + dy * dy;

  if (dist_sq > 0.0f) {
    if (dist_sq >= radius * radius) {
      return contact;
    }

    const float dist = std::sqrt(dist_sq);
    contact.hit = true;
    contact.depth = radius - dist;
    contact.normal = { dx / dist, dy / dist };
    return contact;
  }

  // The center is inside the box; push out through the nearest face.
  const float pen_x = half_width - std::fabs(local.x);
  const float pen_y = half_height - std::fabs(local.y);
  contact.hit = true;
  if (pen_x < pen_y) {
    contact.normal = { local.x < 0.0f ? -1.0f : 1.0f, 0.0f };
    contact.depth = radius + pen_x;
  } else {
    contact.normal = { 0.0f, local.y < 0.0f ? -1.0f : 1.0f };
    contact.depth = radius + pen_y;
  }
  return contact;
}

void keep_deeper(Contact& deepest, const Contact& contact) {
  if (contact.hit && contact.depth > deepest.depth) {
    deepest = contact;
  }
}

Contact deepest_contact(Vector2 pos, float radius) {
  Contact deepest{};

  const int min_col = static_cast<int>(std::floor((pos.x - radius) / BLOCK_SIZE));
  const int max_col = static_cast<int>(std::floor((pos.x + radius) / BLOCK_SIZE));
  const int min_row = static_cast<int>(std::floor((pos.y - radius) / BLOCK_SIZE));
  const int max_row = static_cast<int>(std::floor((pos.y + radius) / BLOCK_SIZE));

  for (int row = min_row; row <= max_row; ++row) {
    for (int col = min_col; col <= max_col; ++col) {
      if (!is_wall_block(col, row)) {
        continue;
      }

      const Vector2 local{ pos.x - (col + 0.5f) * BLOCK_SIZE, pos.y - (row + 0.5f) * BLOCK_SIZE };
      keep_deeper(deepest,
                  box_contact(local, radius, BLOCK_SIZE / 2.0f, BLOCK_SIZE / 2.0f));
    }
  }

  for (const auto& circle : CIRCLE_OBSTACLES) {
    keep_deeper(deepest, circle_contact(pos, radius, circle.center, circle.radius));
  }

  for (const auto& rect : RECT_OBSTACLES) {
    const float c = std::cos(rect.angle);
    const float s = std::sin(rect.angle);
    const float dx = pos.x - rect.center.x;
    const float dy = pos.y - rect.center.y;
    const Vector2 local{ dx * c + dy * s, -dx * s + dy * c };

    Contact contact = box_contact(local, radius, rect.half_width, rect.half_height);
    if (contact.hit) {
      contact.normal = { contact.normal.x * c - contact.normal.y * s,
                         contact.normal.x * s + contact.normal.y * c };
    }
    keep_deeper(deepest, contact);
  }

  return deepest;
}

// Pushes the player out of the deepest overlap. Because the push always follows
// the contact normal, only the blocked part of the movement is cancelled: the
// player keeps sliding along walls and around obstacle corners instead of
// stopping dead.
bool resolve_penetration(Vector2& pos, float radius) {
  const Contact contact = deepest_contact(pos, radius);
  if (!contact.hit) {
    return false;
  }

  pos.x += contact.normal.x * (contact.depth + COLLISION_EPSILON);
  pos.y += contact.normal.y * (contact.depth + COLLISION_EPSILON);
  return true;
}

void move_with_collision(Vector2& pos, Vector2 delta, float radius) {
  for (int i = 0; i < MAX_RESOLVE_ITERATIONS && resolve_penetration(pos, radius); ++i) {
  }

  const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
  if (distance <= 0.0f) {
    return;
  }

  // Small enough substeps to rule out tunneling through 16px blocks.
  const int steps = std::max(1, static_cast<int>(std::ceil(distance / MAX_MOVE_STEP)));
  const Vector2 step{ delta.x / static_cast<float>(steps), delta.y / static_cast<float>(steps) };

  for (int s = 0; s < steps; ++s) {
    pos += step;
    for (int i = 0; i < MAX_RESOLVE_ITERATIONS && resolve_penetration(pos, radius); ++i) {
    }
  }
}

void draw_filled_circle(int cx, int cy, int radius, int color) {
  for (int dy = -radius; dy <= radius; ++dy) {
    int dx = integer_sqrt(radius * radius - dy * dy);
    draw_hline(cx - dx, cy + dy, dx * 2 + 1, color);
  }
}

// One set of scratch buffers for the scanline fill; fills never nest.
constexpr int MAX_POLYGON_VERTICES = LIGHT_RAYS;
int16_t span_head[LCD_HEIGHT];
int16_t span_next[MAX_POLYGON_VERTICES];
int16_t span_end_row[MAX_POLYGON_VERTICES];
int16_t span_active[MAX_POLYGON_VERTICES];


// Draws the dithered light for one scanline. Distance is compared in fixed
// point, and whole 8-pixel groups are written with one lcd_set_byte: the light
// is drawn onto a freshly cleared buffer, so a full byte can be replaced.
void draw_light_span(int y, int x0, int x1, int origin_x, int origin_y) {
  const int dy = (y << SUBPIXEL_BITS) - origin_y;
  const int dy_sq = dy * dy;
  const int* row_dither = BAYER_4X4[y & 3];

  int lo[4];
  int hi[4];
  int lo_min = LCD_WIDTH;
  int lo_max = -1;
  int hi_min = LCD_WIDTH;
  int hi_max = -1;
  bool all_phases = true;

  for (int phase = 0; phase < 4; ++phase) {
    const int budget = DITHER_CUTOFF_FIXED[row_dither[phase]] - dy_sq;
    if (budget <= 0) {
      lo[phase] = LCD_WIDTH + 1;
      hi[phase] = LCD_WIDTH;
      all_phases = false;
      continue;
    }

    const int reach = integer_sqrt(budget - 1);
    lo[phase] = ceil_div(origin_x - reach, SUBPIXEL);
    hi[phase] = floor_div(origin_x + reach, SUBPIXEL);
    lo_min = std::min(lo_min, lo[phase]);
    lo_max = std::max(lo_max, lo[phase]);
    hi_min = std::min(hi_min, hi[phase]);
    hi_max = std::max(hi_max, hi[phase]);
  }

  if (hi_max < 0) {
    return;
  }

  int x = std::max(x0, lo_min);
  const int last_x = std::min(x1, hi_max);

  while (x <= last_x) {
    if ((x & 7) == 0 && x + 7 <= last_x) {
      uint8_t value = 0;
      if (all_phases && x >= lo_max && x + 7 <= hi_min) {
        value = 0xFF;
      } else {
        for (int i = 0; i < 8; ++i) {
          if (x + i >= lo[i & 3] && x + i <= hi[i & 3]) {
            value |= static_cast<uint8_t>(0x80u >> i);
          }
        }
      }
      lcd_set_byte(x >> 3, y, value);
      x += 8;
    } else {
      if (x >= lo[x & 3] && x <= hi[x & 3]) {
        lcd_draw_pixel(x, y, LCD_WHITE);
      }
      ++x;
    }
  }
}

void fill_solid_span(int y, int x0, int x1, int color) {
  const uint8_t value = color == LCD_WHITE ? 0xFF : 0x00;
  int x = x0;
  while (x <= x1) {
    if ((x & 7) == 0 && x + 7 <= x1) {
      lcd_set_byte(x >> 3, y, value);
      x += 8;
    } else {
      lcd_draw_pixel(x, y, color);
      ++x;
    }
  }
}

template <typename SpanWriter>
void fill_polygon(const ScreenPoint* vertices, int count, SpanWriter write_span) {
  if (count < 3 || count > MAX_POLYGON_VERTICES) {
    return;
  }

  for (int y = 0; y < LCD_HEIGHT; ++y) {
    span_head[y] = -1;
  }

  // Bucket every edge by the first scanline it crosses. Each row then only
  // visits the few edges that are on its boundary, instead of testing all of
  // them (which also cost a runtime `%` per test).
  int first_row = LCD_HEIGHT;
  int last_row = -1;
  for (int i = 0; i < count; ++i) {
    const ScreenPoint& a = vertices[i];
    const ScreenPoint& b = vertices[i + 1 == count ? 0 : i + 1];
    const int low_y = std::min(a.y, b.y);
    const int high_y = std::max(a.y, b.y);
    if (low_y == high_y) {
      continue;
    }

    const int start = std::max(ceil_div(low_y - SUBPIXEL / 2, SUBPIXEL), 0);
    const int end = std::min(floor_div(high_y - 1 - SUBPIXEL / 2, SUBPIXEL), LCD_HEIGHT - 1);
    if (start > end) {
      continue;
    }

    span_end_row[i] = static_cast<int16_t>(end);
    span_next[i] = span_head[start];
    span_head[start] = static_cast<int16_t>(i);
    first_row = std::min(first_row, start);
    last_row = std::max(last_row, end);
  }

  int xs[MAX_SPAN_CROSSINGS];
  int active_count = 0;

  for (int y = first_row; y <= last_row; ++y) {
    for (int16_t edge = span_head[y]; edge >= 0; edge = span_next[edge]) {
      span_active[active_count++] = edge;
    }

    int kept = 0;
    for (int i = 0; i < active_count; ++i) {
      if (span_end_row[span_active[i]] >= y) {
        span_active[kept++] = span_active[i];
      }
    }
    active_count = kept;
    if (active_count == 0) {
      continue;
    }

    const int sample_y = (y << SUBPIXEL_BITS) + SUBPIXEL / 2;
    int crossings = 0;
    for (int i = 0; i < active_count && crossings < MAX_SPAN_CROSSINGS; ++i) {
      const int edge = span_active[i];
      const ScreenPoint& a = vertices[edge];
      const ScreenPoint& b = vertices[edge + 1 == count ? 0 : edge + 1];

      // 8.8 slope keeps the multiply inside 32 bits: |slope * offset| is
      // bounded by |dx << 8| because |offset| <= |dy|.
      const int slope = ((b.x - a.x) << 8) / (b.y - a.y);
      xs[crossings++] = a.x + ((slope * (sample_y - a.y)) >> 8);
    }

    // A row only crosses the boundary a handful of times.
    for (int i = 1; i < crossings; ++i) {
      const int value = xs[i];
      int j = i - 1;
      while (j >= 0 && xs[j] > value) {
        xs[j + 1] = xs[j];
        --j;
      }
      xs[j + 1] = value;
    }

    for (int i = 0; i + 1 < crossings; i += 2) {
      const int x0 = std::max(ceil_div(xs[i] - SUBPIXEL / 2, SUBPIXEL), 0);
      const int x1 = std::min(floor_div(xs[i + 1] - SUBPIXEL / 2, SUBPIXEL), LCD_WIDTH - 1);
      if (x0 <= x1) {
        write_span(y, x0, x1);
      }
    }
  }
}

void fill_light_polygon(const ScreenPoint* vertices, int count, int origin_x, int origin_y) {
  fill_polygon(vertices, count, [origin_x, origin_y](int y, int x0, int x1) {
    draw_light_span(y, x0, x1, origin_x, origin_y);
  });
}

void fill_solid_polygon(const ScreenPoint* vertices, int count, int color) {
  fill_polygon(vertices, count,
               [color](int y, int x0, int x1) { fill_solid_span(y, x0, x1, color); });
}

// Walls are not drawn: they are black, the background is black, and the ray
// cast light stops at their faces, so filling them in would only repaint pixels
// that are already dark. That saves roughly 9k pixel writes per frame.

void draw_filled_rotated_rect(Vector2 center, float half_width, float half_height, float angle,
                              int color) {
  const float c = std::cos(angle);
  const float s = std::sin(angle);
  const float extent_x = std::fabs(c) * half_width + std::fabs(s) * half_height;
  const float extent_y = std::fabs(s) * half_width + std::fabs(c) * half_height;

  int min_x = std::max(static_cast<int>(std::floor(center.x - extent_x)), 0);
  int max_x = std::min(static_cast<int>(std::ceil(center.x + extent_x)), LCD_WIDTH - 1);
  int min_y = std::max(static_cast<int>(std::floor(center.y - extent_y)), 0);
  int max_y = std::min(static_cast<int>(std::ceil(center.y + extent_y)), LCD_HEIGHT - 1);

  // 8.8 fixed point: this is a per-pixel loop, keep it away from soft floats.
  const int cos_q = static_cast<int>(c * 256.0f);
  const int sin_q = static_cast<int>(s * 256.0f);
  const int center_x_q = static_cast<int>(center.x * 256.0f);
  const int center_y_q = static_cast<int>(center.y * 256.0f);
  const int half_width_q = static_cast<int>(half_width * 256.0f);
  const int half_height_q = static_cast<int>(half_height * 256.0f);

  for (int y = min_y; y <= max_y; ++y) {
    for (int x = min_x; x <= max_x; ++x) {
      const int dx = (x << 8) - center_x_q;
      const int dy = (y << 8) - center_y_q;
      const int local_x = (dx * cos_q + dy * sin_q) >> 8;
      const int local_y = (-dx * sin_q + dy * cos_q) >> 8;
      if (std::abs(local_x) <= half_width_q && std::abs(local_y) <= half_height_q) {
        draw_pixel(x, y, color);
      }
    }
  }
}

void draw_obstacles(int camera_x, int camera_y) {
  for (const auto& circle : CIRCLE_OBSTACLES) {
    draw_filled_circle(static_cast<int>(std::lround(circle.center.x - camera_x)),
                       static_cast<int>(std::lround(circle.center.y - camera_y)),
                       static_cast<int>(std::lround(circle.radius)), LCD_BLACK);
  }

  for (const auto& rect : RECT_OBSTACLES) {
    Vector2 center{ rect.center.x - camera_x, rect.center.y - camera_y };
    draw_filled_rotated_rect(center, rect.half_width, rect.half_height, rect.angle, LCD_BLACK);
  }
}

float cross2(Vector2 o, Vector2 a, Vector2 b) {
  return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

int convex_hull(Vector2* points, int count, Vector2* hull) {
  if (count < 3) {
    return count;
  }

  std::sort(points, points + count, [](const Vector2& a, const Vector2& b) {
    return a.x < b.x || (a.x == b.x && a.y < b.y);
  });

  int k = 0;
  for (int i = 0; i < count; ++i) {
    while (k >= 2 && cross2(hull[k - 2], hull[k - 1], points[i]) <= 0.0f) {
      --k;
    }
    hull[k++] = points[i];
  }

  for (int i = count - 2, t = k + 1; i >= 0; --i) {
    while (k >= t && cross2(hull[k - 2], hull[k - 1], points[i]) <= 0.0f) {
      --k;
    }
    hull[k++] = points[i];
  }

  if (k > 1) {
    --k;
  }
  return k;
}

void fill_shadow_hull(const Vector2* hull, int hull_count, int camera_x, int camera_y) {
  static ScreenPoint polygon[MAX_SHADOW_HULL_POINTS];
  if (hull_count < 3) {
    return;
  }

  for (int i = 0; i < hull_count; ++i) {
    polygon[i].x = to_subpixel(hull[i].x - camera_x);
    polygon[i].y = to_subpixel(hull[i].y - camera_y);
  }
  fill_solid_polygon(polygon, hull_count, LCD_BLACK);
}

void draw_obstacle_shadows(Vector2 player_world, int camera_x, int camera_y) {
  static Vector2 points[MAX_SHADOW_HULL_POINTS];
  static Vector2 hull[MAX_SHADOW_HULL_POINTS];

  for (const auto& circle : CIRCLE_OBSTACLES) {
    const Vector2 to{ circle.center.x - player_world.x, circle.center.y - player_world.y };
    const float dist = std::sqrt(to.x * to.x + to.y * to.y);
    if (dist <= 0.0f) {
      continue;
    }

    const Vector2 away{ to.x / dist, to.y / dist };
    const float offset = std::min(dist * SHADOW_LENGTH_FACTOR, SHADOW_MAX_OFFSET);
    const float scale = 1.0f + offset * SHADOW_SCALE_PER_OFFSET;
    const Vector2 shadow_center{ circle.center.x + away.x * offset,
                                 circle.center.y + away.y * offset };

    int count = 0;
    for (int i = 0; i < SHADOW_CIRCLE_SEGMENTS; ++i) {
      const float ca = SHADOW_DIRECTIONS[i].x;
      const float sa = SHADOW_DIRECTIONS[i].y;
      points[count++] = { circle.center.x + ca * circle.radius,
                          circle.center.y + sa * circle.radius };
      points[count++] = { shadow_center.x + ca * circle.radius * scale,
                          shadow_center.y + sa * circle.radius * scale };
    }

    const int hull_count = convex_hull(points, count, hull);
    if (hull_count >= 3) {
      fill_shadow_hull(hull, hull_count, camera_x, camera_y);
    }
  }

  for (const auto& rect : RECT_OBSTACLES) {
    const Vector2 to{ rect.center.x - player_world.x, rect.center.y - player_world.y };
    const float dist = std::sqrt(to.x * to.x + to.y * to.y);
    if (dist <= 0.0f) {
      continue;
    }

    const Vector2 away{ to.x / dist, to.y / dist };
    const float offset = std::min(dist * SHADOW_LENGTH_FACTOR, SHADOW_MAX_OFFSET);
    const float scale = 1.0f + offset * SHADOW_SCALE_PER_OFFSET;
    const Vector2 shadow_center{ rect.center.x + away.x * offset,
                                 rect.center.y + away.y * offset };

    const float c = std::cos(rect.angle);
    const float s = std::sin(rect.angle);
    const float local_x[4] = { -rect.half_width, rect.half_width, rect.half_width,
                               -rect.half_width };
    const float local_y[4] = { -rect.half_height, -rect.half_height, rect.half_height,
                               rect.half_height };

    int count = 0;
    for (int i = 0; i < 4; ++i) {
      points[count++] = { rect.center.x + local_x[i] * c - local_y[i] * s,
                          rect.center.y + local_x[i] * s + local_y[i] * c };
      points[count++] = { shadow_center.x + (local_x[i] * scale) * c -
                            (local_y[i] * scale) * s,
                          shadow_center.y + (local_x[i] * scale) * s +
                            (local_y[i] * scale) * c };
    }

    const int hull_count = convex_hull(points, count, hull);
    if (hull_count >= 3) {
      fill_shadow_hull(hull, hull_count, camera_x, camera_y);
    }
  }
}

void draw_light(Vector2 player_world, Vector2 player_screen, int camera_x, int camera_y) {
  static ScreenPoint vertices[LIGHT_RAYS];
  if (!light_tables_ready) {
    init_light_tables();
  }

  const int origin_x = to_ray_subpixel(player_world.x);
  const int origin_y = to_ray_subpixel(player_world.y);
  const int camera_x_ray = camera_x << RAY_SUBPIXEL_BITS;
  const int camera_y_ray = camera_y << RAY_SUBPIXEL_BITS;

  for (int i = 0; i < LIGHT_RAYS; ++i) {
    const RayDir& dir = RAY_DIRECTIONS[i];
    const int dist = raycast_walls(origin_x, origin_y, dir, MAX_LIGHT_DISTANCE_FIXED);
    const int hit_x = origin_x + dir.x * dist / DIR_ONE;
    const int hit_y = origin_y + dir.y * dist / DIR_ONE;
    vertices[i].x = (hit_x - camera_x_ray) << (SUBPIXEL_BITS - RAY_SUBPIXEL_BITS);
    vertices[i].y = (hit_y - camera_y_ray) << (SUBPIXEL_BITS - RAY_SUBPIXEL_BITS);
  }

  fill_light_polygon(vertices, LIGHT_RAYS, to_subpixel(player_screen.x),
                     to_subpixel(player_screen.y));
  draw_obstacle_shadows(player_world, camera_x, camera_y);
}

}  // namespace

DemoScreen::DemoScreen()
  : Screen(), player_pos_{ PLAYER_START } {}

void DemoScreen::init() {
  player_pos_ = PLAYER_START;
  update_camera();
}

void DemoScreen::update_camera() {
  camera_pos_.x = std::clamp(player_pos_.x - LCD_WIDTH / 2.0f, 0.0f,
                             static_cast<float>(MAX_CAMERA_X));
  camera_pos_.y = std::clamp(player_pos_.y - LCD_HEIGHT / 2.0f, 0.0f,
                             static_cast<float>(MAX_CAMERA_Y));
}

Screen* DemoScreen::update() {
  Vector2 dir{};
  if (is_key_down(ESP_KEY_LEFT)) {
    dir.x -= 1.0f;
  }
  if (is_key_down(ESP_KEY_RIGHT)) {
    dir.x += 1.0f;
  }
  if (is_key_down(ESP_KEY_UP)) {
    dir.y -= 1.0f;
  }
  if (is_key_down(ESP_KEY_DOWN)) {
    dir.y += 1.0f;
  }

  if (dir.x != 0.0f || dir.y != 0.0f) {
    const float speed = is_key_down(ESP_KEY_A) ? MOVE_SPEED * RUN_MULTIPLIER : MOVE_SPEED;
    const float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    const float step = speed / length;
    move_with_collision(player_pos_, { dir.x * step, dir.y * step },
                        PLAYER_COLLISION_RADIUS);
  }

  update_camera();

  return this;
}

void DemoScreen::draw() const {
  fill_screen_buffer(LCD_BLACK);
  profile_begin();

  const int camera_x = static_cast<int>(std::lround(camera_pos_.x));
  const int camera_y = static_cast<int>(std::lround(camera_pos_.y));
  const Vector2 player_screen{ player_pos_.x - camera_x, player_pos_.y - camera_y };

  draw_light(player_pos_, player_screen, camera_x, camera_y);
  profile_mark(0);

  draw_obstacles(camera_x, camera_y);
  profile_mark(1);

  const int cx = static_cast<int>(std::lround(player_screen.x));
  const int cy = static_cast<int>(std::lround(player_screen.y));
  draw_filled_circle(cx, cy, static_cast<int>(PLAYER_RADIUS), LCD_BLACK);
  profile_end();
}

void DemoScreen::close() {
}
