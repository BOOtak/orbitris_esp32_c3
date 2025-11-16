#include "game_screen.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

#include "orbital.h"
#include "draw.h"
#include "input.h"
#include "sharp_display.h"


#define TILE_W 8  // px
#define TILE_H 8
#define BLOCK_SIZE 4

constexpr float dist_scale = 6.5E8;
constexpr float star_mass = 1.98855E30;

constexpr float delta_time_max = 3600.0f * 24;
constexpr float delta_time_min = 3600.0f;
constexpr float d_delta_time = delta_time_max - delta_time_min;

constexpr float dist_max = 1.496E11;
constexpr float dist_min = 3.5E8;
constexpr float d_dist = dist_max - dist_min;


static Tetramino I_Block = {
  { { { 0, 0, 0, 0 },
      { 1, 1, 1, 1 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 1, 0 },
      { 0, 0, 1, 0 },
      { 0, 0, 1, 0 },
      { 0, 0, 1, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 1, 1, 1, 1 },
      { 0, 0, 0, 0 } },
    { { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 } } }
};

static Tetramino J_Block = {
  { { { 0, 0, 0, 0 },
      { 1, 0, 0, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 1, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 1, 1, 0, 0 } } }
};

static Tetramino L_Block = {
  { { { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 1, 0 } },
    { { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 1, 1, 1, 0 },
      { 1, 0, 0, 0 } },
    { { 0, 0, 0, 0 },
      { 1, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 } } }
};

static Tetramino* Blocks[] = { &I_Block, &L_Block, &J_Block };

GameScreen::GameScreen()
  : Screen() {}

void GameScreen::init() {
  active_tetramino_ = { 0, { 0.0, 0.0 }, &J_Block };

  planet_state_ = {};
  planet_state_.distance.value = 1.496E11;
  planet_state_.angle.speed = 1.990986E-7;
  delta_time_ = 3600 * 24;

  tilemap_[4][4].occupied = true;
  tilemap_[4][5].occupied = true;
  tilemap_[5][4].occupied = true;
  tilemap_[5][5].occupied = true;
}

Screen* GameScreen::update() {
  float dist_acc = distance_acceleration(planet_state_, star_mass);
  planet_state_.distance.speed = new_value(planet_state_.distance.speed, delta_time_, dist_acc);
  planet_state_.distance.value = new_value(planet_state_.distance.value, delta_time_, planet_state_.distance.speed);

  float angle_acc = angle_acceleratioin(planet_state_);
  planet_state_.angle.speed = new_value(planet_state_.angle.speed, delta_time_, angle_acc);
  planet_state_.angle.value = new_value(planet_state_.angle.value, delta_time_, planet_state_.angle.speed);

  if (is_key_down(KEY_UP)) {
    planet_state_.angle.speed += 1.0E-9;
  }
  if (is_key_down(KEY_DOWN)) {
    planet_state_.angle.speed -= 1.0E-9;
  }

  if (is_key_pressed(KEY_LEFT)) {
    active_tetramino_.rot_index = (active_tetramino_.rot_index + 3) % 4;  // same as -1 % 4
  } else if (is_key_pressed(KEY_RIGHT)) {
    active_tetramino_.rot_index = (active_tetramino_.rot_index + 1) % 4;
  }


  float dist_to_scale = planet_state_.distance.value / dist_scale;
  active_tetramino_.pos.x = star_pos_.x + dist_to_scale * cos(planet_state_.angle.value);
  active_tetramino_.pos.y = star_pos_.y + dist_to_scale * sin(planet_state_.angle.value);

  update_delta_time();

  return this;
}

void GameScreen::draw() {
  lcd_fill_buffer(1);
  draw_tetramino(active_tetramino_);
  draw_tilemap();
}

void GameScreen::draw_tetramino(ActiveTetramino tetramino) {
  int rot_index = tetramino.rot_index;
  int(*block)[4] = tetramino.block->data[rot_index];
  for (size_t i = 0; i < BLOCK_SIZE; i++) {
    for (size_t j = 0; j < BLOCK_SIZE; j++) {
      if (block[i][j] == 0) {
        continue;
      }

      draw_rectangle_lines(tetramino.pos.x + j * TILE_W, tetramino.pos.y + i * TILE_H,
                           TILE_W, TILE_H, 0);
    }
  }
}

bool GameScreen::is_blank(Tile& tile) {
  return !tile.occupied;
}

void GameScreen::draw_tilemap() {
  for (size_t i = 0; i < TILES_X; i++) {
    for (size_t j = 0; j < TILES_Y; j++) {
      if (is_blank(tilemap_[i][j])) {
        continue;
      }

      int posX = (i - TILES_X / 2) * TILE_W + LCD_WIDTH / 2;
      int posY = (j - TILES_Y / 2) * TILE_H + LCD_HEIGHT / 2;
      draw_rectangle_lines(posX, posY, TILE_W, TILE_H, 0);
    }
  }
}

void GameScreen::update_delta_time() {
  float targetDist = planet_state_.distance.value;
  targetDist = std::clamp(targetDist, dist_min, dist_max);

  delta_time_ = delta_time_min + (d_delta_time / d_dist) * (targetDist - dist_min);
}
