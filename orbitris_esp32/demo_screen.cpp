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
constexpr float PLAYER_START_ANGLE = 0.0f;

constexpr float MOVE_SPEED = 1.5f;
constexpr float RUN_MULTIPLIER = 1.8f;
constexpr float ROTATE_SPEED = 0.045f;
constexpr float FAST_ROTATE_MULTIPLIER = 2.5f;

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

constexpr float BEAM_ANGLE = PI / 2.0f;
constexpr float BEAM_HALF_ANGLE = BEAM_ANGLE / 2.0f;
constexpr int BEAM_RAYS = 256;
constexpr float MAX_LIGHT_DISTANCE = 200.0f;
constexpr float RAY_INFINITY = 1.0e30f;

bool is_wall_block(int col, int row) {
  if (col < 0 || col >= MAZE_BLOCK_COLS || row < 0 || row >= MAZE_BLOCK_ROWS) {
    return true;
  }
  return MAZE[row / CELL_BLOCKS][col / CELL_BLOCKS] == '#';
}

float normalize_angle(float angle) {
  constexpr float TWO_PI = 2.0f * PI;
  if (angle > PI) {
    angle -= TWO_PI;
  } else if (angle < -PI) {
    angle += TWO_PI;
  }
  return angle;
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

float raycast_circle(Vector2 origin, Vector2 dir, const CircleObstacle& circle, float max_dist) {
  float ox = origin.x - circle.center.x;
  float oy = origin.y - circle.center.y;
  float b = ox * dir.x + oy * dir.y;
  float c = ox * ox + oy * oy - circle.radius * circle.radius;
  if (c <= 0.0f) {
    return 0.0f;
  }

  float disc = b * b - c;
  if (disc < 0.0f) {
    return max_dist;
  }

  float sqrt_disc = std::sqrt(disc);
  float t = -b - sqrt_disc;
  if (t < 0.0f) {
    t = -b + sqrt_disc;
  }
  if (t < 0.0f || t > max_dist) {
    return max_dist;
  }
  return t;
}

float raycast_rect(Vector2 origin, Vector2 dir, const RotatedRectObstacle& rect, float max_dist) {
  float c = std::cos(rect.angle);
  float s = std::sin(rect.angle);
  float ox = origin.x - rect.center.x;
  float oy = origin.y - rect.center.y;

  float local_ox = ox * c + oy * s;
  float local_oy = -ox * s + oy * c;
  float local_dx = dir.x * c + dir.y * s;
  float local_dy = -dir.x * s + dir.y * c;

  float tmin = -RAY_INFINITY;
  float tmax = RAY_INFINITY;

  if (std::fabs(local_dx) < 1e-6f) {
    if (local_ox < -rect.half_width || local_ox > rect.half_width) {
      return max_dist;
    }
  } else {
    float t1 = (-rect.half_width - local_ox) / local_dx;
    float t2 = (rect.half_width - local_ox) / local_dx;
    if (t1 > t2) {
      std::swap(t1, t2);
    }
    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    if (tmin > tmax) {
      return max_dist;
    }
  }

  if (std::fabs(local_dy) < 1e-6f) {
    if (local_oy < -rect.half_height || local_oy > rect.half_height) {
      return max_dist;
    }
  } else {
    float t1 = (-rect.half_height - local_oy) / local_dy;
    float t2 = (rect.half_height - local_oy) / local_dy;
    if (t1 > t2) {
      std::swap(t1, t2);
    }
    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    if (tmin > tmax) {
      return max_dist;
    }
  }

  if (tmax < 0.0f) {
    return max_dist;
  }
  if (tmin < 0.0f) {
    return 0.0f;
  }
  if (tmin > max_dist) {
    return max_dist;
  }
  return tmin;
}

float raycast_scene(Vector2 origin, Vector2 dir, float max_dist) {
  float best = raycast_walls(origin, dir, max_dist);
  for (const auto& circle : CIRCLE_OBSTACLES) {
    best = std::min(best, raycast_circle(origin, dir, circle, max_dist));
  }
  for (const auto& rect : RECT_OBSTACLES) {
    best = std::min(best, raycast_rect(origin, dir, rect, max_dist));
  }
  return best;
}

bool collides_with_walls(Vector2 pos, float radius) {
  int min_col = static_cast<int>(std::floor((pos.x - radius) / BLOCK_SIZE));
  int max_col = static_cast<int>(std::floor((pos.x + radius) / BLOCK_SIZE));
  int min_row = static_cast<int>(std::floor((pos.y - radius) / BLOCK_SIZE));
  int max_row = static_cast<int>(std::floor((pos.y + radius) / BLOCK_SIZE));

  const float radius_sq = radius * radius - COLLISION_EPSILON;

  for (int row = min_row; row <= max_row; ++row) {
    for (int col = min_col; col <= max_col; ++col) {
      if (!is_wall_block(col, row)) {
        continue;
      }

      float closest_x = std::clamp(pos.x, static_cast<float>(col * BLOCK_SIZE),
                                   static_cast<float>((col + 1) * BLOCK_SIZE));
      float closest_y = std::clamp(pos.y, static_cast<float>(row * BLOCK_SIZE),
                                   static_cast<float>((row + 1) * BLOCK_SIZE));
      float dx = pos.x - closest_x;
      float dy = pos.y - closest_y;
      if (dx * dx + dy * dy < radius_sq) {
        return true;
      }
    }
  }

  return false;
}

bool collides_with_obstacles(Vector2 pos, float radius) {
  for (const auto& circle : CIRCLE_OBSTACLES) {
    float dx = pos.x - circle.center.x;
    float dy = pos.y - circle.center.y;
    float min_dist = radius + circle.radius;
    if (dx * dx + dy * dy < min_dist * min_dist - COLLISION_EPSILON) {
      return true;
    }
  }

  for (const auto& rect : RECT_OBSTACLES) {
    float c = std::cos(rect.angle);
    float s = std::sin(rect.angle);
    float dx = pos.x - rect.center.x;
    float dy = pos.y - rect.center.y;

    float local_x = dx * c + dy * s;
    float local_y = -dx * s + dy * c;
    float closest_x = std::clamp(local_x, -rect.half_width, rect.half_width);
    float closest_y = std::clamp(local_y, -rect.half_height, rect.half_height);
    float ddx = local_x - closest_x;
    float ddy = local_y - closest_y;
    if (ddx * ddx + ddy * ddy < radius * radius - COLLISION_EPSILON) {
      return true;
    }
  }

  return false;
}

bool collides(Vector2 pos, float radius) {
  return collides_with_walls(pos, radius) || collides_with_obstacles(pos, radius);
}

void move_with_collision(Vector2& pos, Vector2 delta, float radius) {
  Vector2 next{ pos.x + delta.x, pos.y };
  if (!collides(next, radius)) {
    pos = next;
  }

  next = { pos.x, pos.y + delta.y };
  if (!collides(next, radius)) {
    pos = next;
  }
}

void draw_filled_circle(int cx, int cy, int radius, int color) {
  for (int dy = -radius; dy <= radius; ++dy) {
    int dx = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - dy * dy)));
    draw_hline(cx - dx, cy + dy, dx * 2 + 1, color);
  }
}

void fill_polygon(const Vector2* vertices, int count, int color) {
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
  float xs[BEAM_RAYS + 2];

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

void draw_light(Vector2 player_world, Vector2 player_screen, float angle, int camera_x,
                int camera_y) {
  constexpr int POLYGON_VERTICES = BEAM_RAYS + 2;
  Vector2 vertices[POLYGON_VERTICES];
  vertices[0] = player_screen;

  for (int i = 0; i <= BEAM_RAYS; ++i) {
    const float offset =
      -BEAM_HALF_ANGLE + (BEAM_ANGLE * static_cast<float>(i)) / static_cast<float>(BEAM_RAYS);
    const Vector2 dir = direction_from_angle(angle + offset);
    const float dist = raycast_scene(player_world, dir, MAX_LIGHT_DISTANCE);
    const Vector2 hit_world{ player_world.x + dir.x * dist, player_world.y + dir.y * dist };
    vertices[i + 1] = { hit_world.x - camera_x, hit_world.y - camera_y };
  }

  fill_polygon(vertices, POLYGON_VERTICES, LCD_WHITE);
}

void draw_direction_hint(int cx, int cy, float angle) {
  Vector2 dir = direction_from_angle(angle);
  Vector2 perp{ -dir.y, dir.x };

  int tip_x = cx + static_cast<int>(dir.x * (PLAYER_RADIUS - 2.0f));
  int tip_y = cy + static_cast<int>(dir.y * (PLAYER_RADIUS - 2.0f));
  int base_x = cx + static_cast<int>(dir.x * (PLAYER_RADIUS - 9.0f));
  int base_y = cy + static_cast<int>(dir.y * (PLAYER_RADIUS - 9.0f));
  int side_x = static_cast<int>(perp.x * 4.0f);
  int side_y = static_cast<int>(perp.y * 4.0f);

  draw_line(cx, cy, tip_x, tip_y, LCD_WHITE);
  draw_line(tip_x, tip_y, base_x + side_x, base_y + side_y, LCD_WHITE);
  draw_line(tip_x, tip_y, base_x - side_x, base_y - side_y, LCD_WHITE);
}

}  // namespace

DemoScreen::DemoScreen()
  : Screen(), player_pos_{ PLAYER_START }, player_angle_{ PLAYER_START_ANGLE } {}

void DemoScreen::init() {
  player_pos_ = PLAYER_START;
  player_angle_ = PLAYER_START_ANGLE;
  update_camera();
}

void DemoScreen::update_camera() {
  camera_pos_.x = std::clamp(player_pos_.x - LCD_WIDTH / 2.0f, 0.0f,
                             static_cast<float>(MAX_CAMERA_X));
  camera_pos_.y = std::clamp(player_pos_.y - LCD_HEIGHT / 2.0f, 0.0f,
                             static_cast<float>(MAX_CAMERA_Y));
}

Screen* DemoScreen::update() {
  const bool fast = is_key_down(ESP_KEY_A);
  const float rotate_speed = fast ? ROTATE_SPEED * FAST_ROTATE_MULTIPLIER : ROTATE_SPEED;

  if (is_key_down(ESP_KEY_LEFT)) {
    player_angle_ -= rotate_speed;
  }
  if (is_key_down(ESP_KEY_RIGHT)) {
    player_angle_ += rotate_speed;
  }
  player_angle_ = normalize_angle(player_angle_);

  int move = 0;
  if (is_key_down(ESP_KEY_UP)) {
    ++move;
  }
  if (is_key_down(ESP_KEY_DOWN)) {
    --move;
  }

  if (move != 0) {
    const float speed = fast ? MOVE_SPEED * RUN_MULTIPLIER : MOVE_SPEED;
    const float distance = speed * static_cast<float>(move);
    const Vector2 dir = direction_from_angle(player_angle_);
    move_with_collision(player_pos_, { dir.x * distance, dir.y * distance },
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

  draw_light(player_pos_, player_screen, player_angle_, camera_x, camera_y);
  draw_maze(camera_x, camera_y);
  draw_obstacles(camera_x, camera_y);

  const int cx = static_cast<int>(std::lround(player_screen.x));
  const int cy = static_cast<int>(std::lround(player_screen.y));
  draw_filled_circle(cx, cy, static_cast<int>(PLAYER_RADIUS), LCD_BLACK);
  draw_direction_hint(cx, cy, player_angle_);
}

void DemoScreen::close() {
}
