#include "world.h"

#include <algorithm>
#include <cmath>

#include "draw.h"
#include "light.h"

const char MAZE[MAZE_ROWS][MAZE_COLS + 1] = {
  "###############",
  "#........#....#",
  "#........###.##",
  "#.............#",
  "####.#####....#",
  "#.#...........#",
  "#........#....#",
  "#.#......#....#",
  "#.#......#.####",
  "##########.#..#",
  "#.#...........#",
  "#.......##.####",
  "#.......#.....#",
  "#.#.....#.....#",
  "###############",
};

bool is_wall_block(int col, int row) {
  if (col < 0 || col >= MAZE_BLOCK_COLS || row < 0 || row >= MAZE_BLOCK_ROWS) {
    return true;
  }
  return MAZE[row / CELL_BLOCKS][col / CELL_BLOCKS] == '#';
}

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

Contact deepest_contact_walls(Vector2 pos, float radius) {
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

  return deepest;
}

bool resolve_penetration_walls(Vector2& pos, float radius) {
  const Contact contact = deepest_contact_walls(pos, radius);
  if (!contact.hit) {
    return false;
  }

  pos.x += contact.normal.x * (contact.depth + COLLISION_EPSILON);
  pos.y += contact.normal.y * (contact.depth + COLLISION_EPSILON);
  return true;
}

void move_with_collision(Vector2& pos, Vector2 delta, float radius) {
  for (int i = 0; i < MAX_RESOLVE_ITERATIONS && resolve_penetration_walls(pos, radius); ++i) {
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
    for (int i = 0; i < MAX_RESOLVE_ITERATIONS && resolve_penetration_walls(pos, radius); ++i) {
    }
  }
}

void draw_filled_circle(int cx, int cy, int radius, int color) {
  for (int dy = -radius; dy <= radius; ++dy) {
    const int dx = integer_sqrt(radius * radius - dy * dy);
    draw_hline(cx - dx, cy + dy, dx * 2 + 1, color);
  }
}

void draw_circle_lines(int cx, int cy, int radius, int color) {
  int x = radius;
  int y = 0;
  int err = 1 - x;

  while (x >= y) {
    draw_pixel(cx + x, cy + y, color);
    draw_pixel(cx + y, cy + x, color);
    draw_pixel(cx - y, cy + x, color);
    draw_pixel(cx - x, cy + y, color);
    draw_pixel(cx - x, cy - y, color);
    draw_pixel(cx - y, cy - x, color);
    draw_pixel(cx + y, cy - x, color);
    draw_pixel(cx + x, cy - y, color);

    ++y;
    if (err < 0) {
      err += 2 * y + 1;
    } else {
      --x;
      err += 2 * (y - x) + 1;
    }
  }
}
