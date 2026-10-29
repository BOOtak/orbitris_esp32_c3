#include "demo_screen.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "const.h"
#include "draw.h"
#include "input.h"

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

constexpr float COLLISION_EPSILON = 0.01f;

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
constexpr float RAY_INFINITY = 1.0e30f;

constexpr int BAYER_4X4[4][4] = {
  { 0, 8, 2, 10 },
  { 12, 4, 14, 6 },
  { 3, 11, 1, 9 },
  { 15, 7, 13, 5 },
};

constexpr float dither_cutoff(int bayer) {
  const float threshold = (static_cast<float>(bayer) + 0.5f) / 16.0f;
  return LIGHT_FULL_DISTANCE_SQ +
         (1.0f - threshold) * (MAX_LIGHT_DISTANCE_SQ - LIGHT_FULL_DISTANCE_SQ);
}

constexpr float DITHER_DISTANCE_SQ[16] = {
  dither_cutoff(0),  dither_cutoff(1),  dither_cutoff(2),  dither_cutoff(3),
  dither_cutoff(4),  dither_cutoff(5),  dither_cutoff(6),  dither_cutoff(7),
  dither_cutoff(8),  dither_cutoff(9),  dither_cutoff(10), dither_cutoff(11),
  dither_cutoff(12), dither_cutoff(13), dither_cutoff(14), dither_cutoff(15),
};

bool is_wall_block(int col, int row) {
  if (col < 0 || col >= MAZE_BLOCK_COLS || row < 0 || row >= MAZE_BLOCK_ROWS) {
    return true;
  }
  return MAZE[row / CELL_BLOCKS][col / CELL_BLOCKS] == '#';
}

Vector2 direction_from_angle(float angle) {
  return { std::sin(angle), -std::cos(angle) };
}

float raycast_walls(Vector2 origin, Vector2 dir, float max_dist) {
  int col = static_cast<int>(std::floor(origin.x / BLOCK_SIZE));
  int row = static_cast<int>(std::floor(origin.y / BLOCK_SIZE));
  if (is_wall_block(col, row)) {
    return 0.0f;
  }

  int step_x = (dir.x > 0.0f) - (dir.x < 0.0f);
  int step_y = (dir.y > 0.0f) - (dir.y < 0.0f);
  float t_delta_x = step_x != 0 ? std::fabs(BLOCK_SIZE / dir.x) : RAY_INFINITY;
  float t_delta_y = step_y != 0 ? std::fabs(BLOCK_SIZE / dir.y) : RAY_INFINITY;

  float t_max_x = RAY_INFINITY;
  if (step_x > 0) {
    t_max_x = ((col + 1) * BLOCK_SIZE - origin.x) / dir.x;
  } else if (step_x < 0) {
    t_max_x = (col * BLOCK_SIZE - origin.x) / dir.x;
  }

  float t_max_y = RAY_INFINITY;
  if (step_y > 0) {
    t_max_y = ((row + 1) * BLOCK_SIZE - origin.y) / dir.y;
  } else if (step_y < 0) {
    t_max_y = (row * BLOCK_SIZE - origin.y) / dir.y;
  }

  while (true) {
    float t;
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
    int dx = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - dy * dy)));
    draw_hline(cx - dx, cy + dy, dx * 2 + 1, color);
  }
}

void fill_light_polygon(const Vector2* vertices, int count, Vector2 origin) {
  if (count < 3) {
    return;
  }

  float min_y = vertices[0].y;
  float max_y = vertices[0].y;
  for (int i = 1; i < count; ++i) {
    min_y = std::min(min_y, vertices[i].y);
    max_y = std::max(max_y, vertices[i].y);
  }

  int y_start = std::max(static_cast<int>(std::ceil(min_y)), 0);
  int y_end = std::min(static_cast<int>(std::floor(max_y)), LCD_HEIGHT - 1);
  static float xs[LIGHT_RAYS + 2];

  for (int y = y_start; y <= y_end; ++y) {
    float fy = static_cast<float>(y) + 0.5f;
    int count_x = 0;

    for (int i = 0; i < count; ++i) {
      const Vector2& a = vertices[i];
      const Vector2& b = vertices[(i + 1) % count];
      if ((fy >= a.y && fy < b.y) || (fy >= b.y && fy < a.y)) {
        float t = (fy - a.y) / (b.y - a.y);
        xs[count_x++] = a.x + (b.x - a.x) * t;
      }
    }

    if (count_x < 2) {
      continue;
    }

    std::sort(xs, xs + count_x);

    for (int i = 0; i + 1 < count_x; i += 2) {
      int x0 = std::max(static_cast<int>(std::ceil(xs[i] - 0.5f)), 0);
      int x1 = std::min(static_cast<int>(std::floor(xs[i + 1] - 0.5f)), LCD_WIDTH - 1);
      if (x0 > x1) {
        continue;
      }

      const float dy = static_cast<float>(y) - origin.y;
      const float dy_sq = dy * dy;
      for (int x = x0; x <= x1; ++x) {
        const float dx = static_cast<float>(x) - origin.x;
        const float dist_sq = dx * dx + dy_sq;
        if (dist_sq >= MAX_LIGHT_DISTANCE_SQ) {
          continue;
        }

        const int bayer = BAYER_4X4[y & 3][x & 3];
        if (dist_sq >= DITHER_DISTANCE_SQ[bayer]) {
          continue;
        }

        draw_pixel(x, y, LCD_WHITE);
      }
    }
  }
}

void draw_maze(int camera_x, int camera_y) {
  int first_col = std::max(camera_x / BLOCK_SIZE, 0);
  int last_col = std::min((camera_x + LCD_WIDTH - 1) / BLOCK_SIZE, MAZE_BLOCK_COLS - 1);
  int first_row = std::max(camera_y / BLOCK_SIZE, 0);
  int last_row = std::min((camera_y + LCD_HEIGHT - 1) / BLOCK_SIZE, MAZE_BLOCK_ROWS - 1);

  for (int row = first_row; row <= last_row; ++row) {
    for (int col = first_col; col <= last_col; ++col) {
      if (!is_wall_block(col, row)) {
        continue;
      }

      // Leave a one-pixel gap on the right and bottom edges so neighbouring
      // blocks stay distinguishable.
      int x0 = std::max(col * BLOCK_SIZE - camera_x, 0);
      int y0 = std::max(row * BLOCK_SIZE - camera_y, 0);
      int x1 = std::min((col + 1) * BLOCK_SIZE - camera_x - 1, LCD_WIDTH);
      int y1 = std::min((row + 1) * BLOCK_SIZE - camera_y - 1, LCD_HEIGHT);
      if (x0 >= x1 || y0 >= y1) {
        continue;
      }

      draw_rectangle(x0, y0, x1 - x0, y1 - y0, LCD_BLACK);
    }
  }
}

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

  for (int y = min_y; y <= max_y; ++y) {
    for (int x = min_x; x <= max_x; ++x) {
      const float dx = static_cast<float>(x) - center.x;
      const float dy = static_cast<float>(y) - center.y;
      const float local_x = dx * c + dy * s;
      const float local_y = -dx * s + dy * c;
      if (std::fabs(local_x) <= half_width && std::fabs(local_y) <= half_height) {
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

void fill_solid_polygon(const Vector2* vertices, int count, int color) {
  if (count < 3) {
    return;
  }

  float min_y = vertices[0].y;
  float max_y = vertices[0].y;
  for (int i = 1; i < count; ++i) {
    min_y = std::min(min_y, vertices[i].y);
    max_y = std::max(max_y, vertices[i].y);
  }

  int y_start = std::max(static_cast<int>(std::ceil(min_y)), 0);
  int y_end = std::min(static_cast<int>(std::floor(max_y)), LCD_HEIGHT - 1);
  static float xs[MAX_SHADOW_HULL_POINTS];

  for (int y = y_start; y <= y_end; ++y) {
    float fy = static_cast<float>(y) + 0.5f;
    int count_x = 0;

    for (int i = 0; i < count; ++i) {
      const Vector2& a = vertices[i];
      const Vector2& b = vertices[(i + 1) % count];
      if ((fy >= a.y && fy < b.y) || (fy >= b.y && fy < a.y)) {
        float t = (fy - a.y) / (b.y - a.y);
        xs[count_x++] = a.x + (b.x - a.x) * t;
      }
    }

    if (count_x < 2) {
      continue;
    }

    std::sort(xs, xs + count_x);
    for (int i = 0; i + 1 < count_x; i += 2) {
      int x0 = std::max(static_cast<int>(std::ceil(xs[i] - 0.5f)), 0);
      int x1 = std::min(static_cast<int>(std::floor(xs[i + 1] - 0.5f)), LCD_WIDTH - 1);
      if (x0 <= x1) {
        draw_hline(x0, y, x1 - x0 + 1, color);
      }
    }
  }
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
      const float a =
        2.0f * PI * static_cast<float>(i) / static_cast<float>(SHADOW_CIRCLE_SEGMENTS);
      const float ca = std::cos(a);
      const float sa = std::sin(a);
      points[count++] = { circle.center.x + ca * circle.radius,
                          circle.center.y + sa * circle.radius };
      points[count++] = { shadow_center.x + ca * circle.radius * scale,
                          shadow_center.y + sa * circle.radius * scale };
    }

    const int hull_count = convex_hull(points, count, hull);
    if (hull_count >= 3) {
      for (int i = 0; i < hull_count; ++i) {
        hull[i].x -= camera_x;
        hull[i].y -= camera_y;
      }
      fill_solid_polygon(hull, hull_count, LCD_BLACK);
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
      for (int i = 0; i < hull_count; ++i) {
        hull[i].x -= camera_x;
        hull[i].y -= camera_y;
      }
      fill_solid_polygon(hull, hull_count, LCD_BLACK);
    }
  }
}

void draw_light(Vector2 player_world, Vector2 player_screen, int camera_x, int camera_y) {
  constexpr int POLYGON_VERTICES = LIGHT_RAYS;
  static Vector2 vertices[POLYGON_VERTICES];

  for (int i = 0; i < LIGHT_RAYS; ++i) {
    const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(LIGHT_RAYS);
    const Vector2 dir = direction_from_angle(angle);
    const float dist = raycast_walls(player_world, dir, MAX_LIGHT_DISTANCE);
    const Vector2 hit_world{ player_world.x + dir.x * dist, player_world.y + dir.y * dist };
    vertices[i] = { hit_world.x - camera_x, hit_world.y - camera_y };
  }

  fill_light_polygon(vertices, POLYGON_VERTICES, player_screen);
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

  const int camera_x = static_cast<int>(std::lround(camera_pos_.x));
  const int camera_y = static_cast<int>(std::lround(camera_pos_.y));
  const Vector2 player_screen{ player_pos_.x - camera_x, player_pos_.y - camera_y };

  draw_light(player_pos_, player_screen, camera_x, camera_y);
  draw_maze(camera_x, camera_y);
  draw_obstacles(camera_x, camera_y);

  const int cx = static_cast<int>(std::lround(player_screen.x));
  const int cy = static_cast<int>(std::lround(player_screen.y));
  draw_filled_circle(cx, cy, static_cast<int>(PLAYER_RADIUS), LCD_BLACK);
}

void DemoScreen::close() {
}
