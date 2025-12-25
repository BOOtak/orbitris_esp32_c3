#include "explosion.h"

#include <cstring>

#include "draw.h"
#include "tilemap.h"

struct ExplodingTile {
  Vector2 pos;
  Vector2 speed;
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
        cell.speed = (cell.pos + tile_center_offset - center) * 0.3f;
      }
    }
  }
}

bool update_explosion() {
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      if (explosion_[i][j].occupied) {
        explosion_[i][j].pos += explosion_[i][j].speed;
      }
    }
  }

  return true;
}

void draw_explosion() {
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      if (explosion_[i][j].occupied) {
        const Vector2& pos = explosion_[i][j].pos;
        draw_tile(pos.x, pos.y, TILE_W);
      }
    }
  }
}
