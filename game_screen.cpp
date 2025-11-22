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


GameScreen::GameScreen()
  : Screen() {}

void GameScreen::init() {
  active_tetramino_ = { 0, { 0.0, 0.0 }, &J_Block };
  next_tetramino_ = {};
  sliding_tetramino_ = {};

  reset_planet_state();
  delta_time_ = 3600 * 24;

  tilemap_.init();
}

Screen* GameScreen::update() {
  Rectangle collision{};
  int sim_runs = get_resolution(planet_state_);
  for (int iter = 0; iter < sim_runs; iter++) {
    update_planet_state(planet_state_, delta_time_ / sim_runs, star_mass);
    active_tetramino_.pos = state_to_coords(planet_state_, dist_scale, star_pos_);

    collision = tilemap_.intersect_tiles(active_tetramino_);

    if (collision.width > 0 && collision.height > 0) {
      break;
    }
  }

  if (collision.width > 0 && collision.height > 0) {
    tilemap_.place_tetramino(active_tetramino_);
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
  tilemap_.draw();
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
