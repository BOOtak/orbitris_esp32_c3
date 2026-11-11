#include "light.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "const.h"
#include "lcd.h"
#include "world.h"

namespace {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float MAX_LIGHT_DISTANCE = 200.0f;
constexpr float LIGHT_FULL_DISTANCE = 64.0f;
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
constexpr int MAX_POLYGON_VERTICES = LIGHT_RAYS;

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
    LIGHT_FULL_DISTANCE_SQ + (1.0f - threshold) * (MAX_LIGHT_DISTANCE_SQ - LIGHT_FULL_DISTANCE_SQ);
  return static_cast<int>(cutoff * static_cast<float>(SUBPIXEL * SUBPIXEL));
}

// Squared light cutoff per Bayer cell, scaled by SUBPIXEL^2 to match the
// fixed-point distance.
constexpr int DITHER_CUTOFF_FIXED[16] = {
  dither_cutoff_fixed(0),
  dither_cutoff_fixed(1),
  dither_cutoff_fixed(2),
  dither_cutoff_fixed(3),
  dither_cutoff_fixed(4),
  dither_cutoff_fixed(5),
  dither_cutoff_fixed(6),
  dither_cutoff_fixed(7),
  dither_cutoff_fixed(8),
  dither_cutoff_fixed(9),
  dither_cutoff_fixed(10),
  dither_cutoff_fixed(11),
  dither_cutoff_fixed(12),
  dither_cutoff_fixed(13),
  dither_cutoff_fixed(14),
  dither_cutoff_fixed(15),
};

struct RayDir {
  int x;
  int y;
};

RayDir RAY_DIRECTIONS[LIGHT_RAYS];
bool light_tables_ready = false;

void init_light_tables() {
  for (int i = 0; i < LIGHT_RAYS; ++i) {
    const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(LIGHT_RAYS);
    const float dir_x = std::sin(angle);
    const float dir_y = -std::cos(angle);
    RAY_DIRECTIONS[i].x = static_cast<int>(dir_x * DIR_ONE + (dir_x < 0.0f ? -0.5f : 0.5f));
    RAY_DIRECTIONS[i].y = static_cast<int>(dir_y * DIR_ONE + (dir_y < 0.0f ? -0.5f : 0.5f));
  }

  light_tables_ready = true;
}

int to_ray_subpixel(float value) {
  return static_cast<int>(value * RAY_SUBPIXEL + (value < 0.0f ? -0.5f : 0.5f));
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

// One set of scratch buffers for the scanline fill; fills never nest.
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

// Adds the dithered lit bits of one span into a per-row 1-bit mask.
void add_lit_bits(int y, int x0, int x1, int origin_x, int origin_y, uint8_t* row_mask) {
  const int dy = (y << SUBPIXEL_BITS) - origin_y;
  const int dy_sq = dy * dy;
  const int* row_dither = BAYER_4X4[y & 3];

  int lo[4];
  int hi[4];
  int lo_min = LCD_WIDTH;
  int hi_max = -1;

  for (int phase = 0; phase < 4; ++phase) {
    const int budget = DITHER_CUTOFF_FIXED[row_dither[phase]] - dy_sq;
    if (budget <= 0) {
      lo[phase] = LCD_WIDTH + 1;
      hi[phase] = LCD_WIDTH;
      continue;
    }

    const int reach = integer_sqrt(budget - 1);
    lo[phase] = ceil_div(origin_x - reach, SUBPIXEL);
    hi[phase] = floor_div(origin_x + reach, SUBPIXEL);
    lo_min = std::min(lo_min, lo[phase]);
    hi_max = std::max(hi_max, hi[phase]);
  }

  if (hi_max < 0) {
    return;
  }

  const int begin = std::max(x0, lo_min);
  const int end = std::min(x1, hi_max);
  if (begin > end) {
    return;
  }

  for (int cx = begin >> 3; cx <= (end >> 3); ++cx) {
    uint8_t value = 0;
    const int base = cx << 3;
    for (int i = 0; i < 8; ++i) {
      const int px = base + i;
      if (px < x0 || px > x1) {
        continue;
      }
      if (px >= lo[px & 3] && px <= hi[px & 3]) {
        value |= static_cast<uint8_t>(0x80u >> i);
      }
    }
    row_mask[cx] |= value;
  }
}

enum class FillMode {
  Light,
  Solid,
  Darkness,
};

void fill_polygon(const ScreenPoint* vertices, int count, FillMode mode, int origin_x,
                  int origin_y, int color) {
  if (count < 3 || count > MAX_POLYGON_VERTICES) {
    return;
  }

  for (int y = 0; y < LCD_HEIGHT; ++y) {
    span_head[y] = -1;
  }

  // Bucket every edge by the first scanline it crosses. Each row then only
  // visits the few edges that are on its boundary.
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
  }

  int xs[MAX_SPAN_CROSSINGS];
  int active_count = 0;
  uint8_t row_mask[LCD_WIDTH / 8];

  for (int y = 0; y < LCD_HEIGHT; ++y) {
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

    if (mode == FillMode::Darkness) {
      std::memset(row_mask, 0, sizeof(row_mask));
      for (int i = 0; i + 1 < crossings; i += 2) {
        const int x0 = std::max(ceil_div(xs[i] - SUBPIXEL / 2, SUBPIXEL), 0);
        const int x1 = std::min(floor_div(xs[i + 1] - SUBPIXEL / 2, SUBPIXEL), LCD_WIDTH - 1);
        if (x0 <= x1) {
          add_lit_bits(y, x0, x1, origin_x, origin_y, row_mask);
        }
      }

      for (int cx = 0; cx < LCD_WIDTH / 8; ++cx) {
        if (row_mask[cx] != 0xFF) {
          lcd_and_byte(cx, y, row_mask[cx]);
        }
      }
      continue;
    }

    for (int i = 0; i + 1 < crossings; i += 2) {
      const int x0 = std::max(ceil_div(xs[i] - SUBPIXEL / 2, SUBPIXEL), 0);
      const int x1 = std::min(floor_div(xs[i + 1] - SUBPIXEL / 2, SUBPIXEL), LCD_WIDTH - 1);
      if (x0 > x1) {
        continue;
      }

      if (mode == FillMode::Light) {
        draw_light_span(y, x0, x1, origin_x, origin_y);
      } else {
        fill_solid_span(y, x0, x1, color);
      }
    }
  }
}

}  // namespace

int floor_div(int value, int divisor) {
  return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
}

int ceil_div(int value, int divisor) {
  return -floor_div(-value, divisor);
}

int to_subpixel(float value) {
  return static_cast<int>(value * SUBPIXEL + (value < 0.0f ? -0.5f : 0.5f));
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

void build_light_polygon(Vector2 player_world, int camera_x, int camera_y,
                         ScreenPoint* out_vertices) {
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
    out_vertices[i].x = (hit_x - camera_x_ray) << (SUBPIXEL_BITS - RAY_SUBPIXEL_BITS);
    out_vertices[i].y = (hit_y - camera_y_ray) << (SUBPIXEL_BITS - RAY_SUBPIXEL_BITS);
  }
}

void fill_light_polygon(const ScreenPoint* vertices, int count, int origin_x, int origin_y) {
  fill_polygon(vertices, count, FillMode::Light, origin_x, origin_y, LCD_WHITE);
}

void fill_solid_polygon(const ScreenPoint* vertices, int count, int color) {
  fill_polygon(vertices, count, FillMode::Solid, 0, 0, color);
}

void draw_darkness_polygon(const ScreenPoint* vertices, int count, int origin_x, int origin_y) {
  fill_polygon(vertices, count, FillMode::Darkness, origin_x, origin_y, LCD_BLACK);
}
