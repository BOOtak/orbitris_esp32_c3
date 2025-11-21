#include "game_screen.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

#include "draw.h"
#include "input.h"

// #include <Arduino.h>

constexpr float dist_scale = 6.5E8;
constexpr float star_mass = 1.98855E30;

constexpr int resolution_min = 1;  // times per frame
constexpr int resolution_max = 10;
constexpr int d_resolution = resolution_max - resolution_min;

constexpr float dist_max = 1.496E11;
constexpr float dist_min = 3.5E8;
constexpr float d_dist = dist_max - dist_min;

// Tilemap location and dimensions
constexpr int centerX = LCD_WIDTH / 2;
constexpr int centerY = LCD_HEIGHT / 2;
constexpr int tileMapPosX = (LCD_WIDTH - TILES_X * TILE_W) / 2;
constexpr int tileMapPosY = (LCD_HEIGHT - TILES_Y * TILE_H) / 2;
constexpr int tileMapWidth = TILES_X * TILE_W;
constexpr int tileMapHeight = TILES_Y * TILE_H;
constexpr Rectangle tileMapRect = { tileMapPosX, tileMapPosY, tileMapWidth, tileMapHeight };

GameScreen::GameScreen()
  : Screen() {}

void GameScreen::init() {
  active_tetramino_ = { 0, { 0.0, 0.0 }, &J_Block };
  next_tetramino_ = {};
  sliding_tetramino_ = {};

  reset_planet_state();
  delta_time_ = 3600 * 24;

  tilemap_[TILES_X / 2 - 1][TILES_Y / 2 - 1].occupied = true;
  tilemap_[TILES_X / 2 - 1][TILES_Y / 2].occupied = true;
  tilemap_[TILES_X / 2][TILES_Y / 2 - 1].occupied = true;
  tilemap_[TILES_X / 2][TILES_Y / 2].occupied = true;
}

Screen* GameScreen::update() {
  Rectangle collision{};
  int sim_runs = get_resolution(planet_state_);
  for (int iter = 0; iter < sim_runs; iter++) {
    update_planet_state(planet_state_, delta_time_ / sim_runs, star_mass);
    active_tetramino_.pos = state_to_coords(planet_state_, dist_scale, star_pos_);

    collision = intersect_tiles(active_tetramino_);

    if (collision.width > 0 && collision.height > 0) {
      break;
    }
  }

  if (collision.width > 0 && collision.height > 0) {
    place_tetramino(active_tetramino_);
    //   // sliding_tetramino.block = active_tetramino.block;
    //   // sliding_tetramino.pos.x = active_tetramino.pos.x;
    //   // sliding_tetramino.pos.y = active_tetramino.pos.y;
    //   // sliding_tetramino.rot_index = active_tetramino.rot_index;
    //   // sliding_tetramino.progress = 1.1;  // to check where to slide it first
    reset_planet_state();
    generate_next_tetramino();
    active_tetramino_.pos = state_to_coords(planet_state_, dist_scale, star_pos_);
  }

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

  return this;
}

void GameScreen::draw() {
  lcd_fill_buffer(1);
  draw_tetramino(active_tetramino_);
  draw_tilemap();
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

/**
 * @brief Return resolution of the simulation (runs per frame). Resolution
 * depends on the distance from the "star". Higher resolution = more accurate
 * simulation. Currently, 1 run of orbital mechahics calculation costs 
 * approx. 20 us
 */
int GameScreen::get_resolution(const PlanetState& planet) {
  float targetDist = std::clamp(planet.distance.value, dist_min, dist_max);
  return (int)remap(targetDist, dist_min, dist_max, resolution_max, resolution_min);
}

Rectangle GameScreen::intersect_tiles(const ActiveTetramino& block) {
  // TODO: optimize and check collisions only with tiles surrounding the
  // tilemap. Or don't do that, it should work just fine as-is

  Rectangle blockRect{
    block.pos.x - block.block->center.x * TILE_W,
    block.pos.y - block.block->center.y * TILE_H,
    TILE_W * BLOCK_SIZE,
    TILE_H * BLOCK_SIZE
  };

  if (check_collision_recs(tileMapRect, blockRect)) {
    Rectangle tileRect{ 0.0f, 0.0f, TILE_W, TILE_H };

    for (size_t i = 0; i < TILES_X; i++) {
      for (size_t j = 0; j < TILES_Y; j++) {
        if (is_blank(tilemap_[i][j])) {
          continue;
        }

        int posX = (i - TILES_X / 2) * TILE_W + centerX;
        int posY = (j - TILES_Y / 2) * TILE_H + centerY;
        tileRect.x = posX;
        tileRect.y = posY;

        if (check_collision_recs(tileRect, blockRect)) {
          // Serial.printf("Hit tile %lu %lu\n", i, j);
          int rot_index = block.rot_index;
          uint8_t(*data)[4] = block.block->data[rot_index];
          Rectangle targetRect = { .x = 0, .y = 0, .width = TILE_W, .height = TILE_H };
          for (size_t y = 0; y < BLOCK_SIZE; y++) {
            for (size_t x = 0; x < BLOCK_SIZE; x++) {
              if (data[y][x] == 0) {
                continue;
              }

              targetRect.x = blockRect.x + x * TILE_W;
              targetRect.y = blockRect.y + y * TILE_H;

              if (check_collision_recs(tileRect, targetRect)) {
                // Serial.printf("Collision!\n");
                return get_collision_rec(tileRect, targetRect);
              }
            }
          }
        }
      }
    }
  }

  return {};
}


void GameScreen::get_tetramino_tilemap_pos_corner(const ActiveTetramino& block, int* x, int* y) {
  size_t idx = 0;
  float deltaX = block.pos.x - block.block->center.x * TILE_W - tileMapPosX;
  float deltaY = block.pos.y - block.block->center.y * TILE_H - tileMapPosY;
  int ix = (int)round(deltaX / TILE_W);
  int iy = (int)round(deltaY / TILE_H);

  *x = ix;
  *y = iy;
}

void GameScreen::get_tetramino_tilemap_pos(const ActiveTetramino& block, int (*coords)[2] /* int[4][2] */) {
  size_t idx = 0;
  float deltaX = block.pos.x - block.block->center.x * TILE_W - tileMapPosX;
  float deltaY = block.pos.y - block.block->center.y * TILE_H - tileMapPosY;
  int ix = 0, iy = 0;
  get_tetramino_tilemap_pos_corner(block, &ix, &iy);

  // TODO: Check bounds!
  uint8_t(*data)[BLOCK_SIZE] = block.block->data[block.rot_index];
  for (size_t y = 0; y < BLOCK_SIZE; y++) {
    for (size_t x = 0; x < BLOCK_SIZE; x++) {
      if (data[y][x] == 0) {
        continue;
      }

      coords[idx][0] = ix + x;
      coords[idx][1] = iy + y;
      idx++;
    }
  }
}

void GameScreen::place_tetramino(const ActiveTetramino& block) {
  int coords[BLOCK_SIZE][2]{};
  get_tetramino_tilemap_pos(block, coords);
  for (size_t i = 0; i < BLOCK_SIZE; i++) {
    // Serial.printf("Add %d %d\n", coords[i][0], coords[i][1]);
    // tilemap[coords[i][0]][coords[i][1]] = block.block->color;
    tilemap_[coords[i][0]][coords[i][1]].occupied = true;
  }

  // CheckRows();
  // CheckTilesOOB();
}

void GameScreen::reset_planet_state() {
  planet_state_ = {};
  planet_state_.distance.value = 1.496E11;
  planet_state_.angle.speed = 1.990986E-7;
}

void GameScreen::generate_next_tetramino() {
  // TODO: Blocks pool!
  if (next_tetramino_.block == NULL) {
    active_tetramino_.block = get_random_block();
  } else {
    active_tetramino_.block = next_tetramino_.block;
  }

  next_tetramino_.block = get_random_block();
  active_tetramino_.rot_index = 0;
}
