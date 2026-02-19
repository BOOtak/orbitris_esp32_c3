#include "explosion.h"

#include <cstring>

#include "draw.h"
#include "tilemap.h"
#include "transition.h"

constexpr float SPEED_THRESHOLD_SQUARED = 0.1f;

struct ExplodingTile {
  Vector2 pos;
  Vector2 speed;
  Vector2 initSpeed;
  bool occupied;
};

ExplodingTile explosion_[TILES_X][TILES_Y]{};

void init_explosion(const Tilemap& map, const Vector2& center) {
  std::memset(explosion_, 0, sizeof(explosion_));
  Vector2 tile_center_offset{ TILE_W / 2, TILE_H / 2 };
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      if (!map.is_blank(i, j)) {
        ExplodingTile& cell = explosion_[i][j];
        cell.occupied = true;
        cell.pos = map.get_tile_pos(i, j);
        cell.initSpeed = cell.speed = (cell.pos + tile_center_offset - center) * get_random_value(0.2f, 0.4f);
      }
    }
  }
}

bool update_explosion() {
  bool all_stopped = true;
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      auto& cell = explosion_[i][j];

      if (cell.occupied) {
        cell.pos += cell.speed;
        cell.speed *= 0.98f;
        if (vector2_square_length(cell.speed) > SPEED_THRESHOLD_SQUARED) {
          all_stopped = false;
        }
      }
    }
  }

  return all_stopped;
}

void draw_explosion() {
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      const auto& cell = explosion_[i][j];
      if (cell.occupied) {
        size_t mask_index = remap(cell.speed.x, 0.0f, cell.initSpeed.x, 0.0f, get_masks_count() - 1);
        const Vector2& pos = explosion_[i][j].pos;
        begin_mask(get_mask(mask_index));
        draw_tile(pos.x, pos.y, TILE_W);
        end_mask();
      }
    }
  }
}
