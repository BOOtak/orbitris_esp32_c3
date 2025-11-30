#include "game_screen.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

#include "draw.h"
#include "input.h"
#include "trace.h"

#include <array>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr float DIST_SCALE = 6.5E8f;
constexpr float STAR_MASS = 1.98855E30f;

constexpr int resolution_min = 1;  // times per frame
constexpr int resolution_max = 10;
constexpr int d_resolution = resolution_max - resolution_min;

constexpr float dist_max = 1.496E11f;
constexpr float dist_min = 3.5E8f;
constexpr float d_dist = dist_max - dist_min;

constexpr auto trajectory_size = 50;

constexpr float PROGRESS_SPEED = 0.05f;
constexpr float DIST_THRESHOLD = 0.0001f;

const uint8_t patterns[] = { 0x00, 0x11, 0x24, 0x55, 0xd8, 0xee, 0xf0, 0xf8 };
const uint8_t pattern_sizes[] = { 8, 8, 6, 8, 6, 8, 5, 6 };
constexpr auto patterns_count = std::size(patterns);

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
  tilemap_.update();

  Rectangle collision{};
  int sim_runs = get_resolution(planet_state_);
  for (int iter = 0; iter < sim_runs; iter++) {
    update_planet_state(planet_state_, delta_time_ / sim_runs, STAR_MASS);
    active_tetramino_.pos = state_to_coords(planet_state_, DIST_SCALE, star_pos_);

    collision = tilemap_.intersect_tiles(active_tetramino_);

    if (collision.width > 0 && collision.height > 0) {
      break;
    }
  }

  if (collision.width > 0 && collision.height > 0) {
    sliding_tetramino_.block = active_tetramino_.block;
    sliding_tetramino_.pos.x = active_tetramino_.pos.x;
    sliding_tetramino_.pos.y = active_tetramino_.pos.y;
    sliding_tetramino_.rot_index = active_tetramino_.rot_index;
    sliding_tetramino_.progress = 1.1f;  // to check where to slide it first
    reset_planet_state();
    generate_next_tetramino();
    active_tetramino_.pos = state_to_coords(planet_state_, DIST_SCALE, star_pos_);
  }

  update_sliding_tetramino(sliding_tetramino_);

  if (is_key_down(ESP_KEY_UP)) {
    planet_state_.angle.speed += 1.0E-9f;
  }
  if (is_key_down(ESP_KEY_DOWN)) {
    planet_state_.angle.speed -= 1.0E-9f;
  }

  if (is_key_pressed(ESP_KEY_LEFT)) {
    active_tetramino_.rot_index = (active_tetramino_.rot_index + 3) % 4;  // same as -1 % 4
  } else if (is_key_pressed(ESP_KEY_RIGHT)) {
    active_tetramino_.rot_index = (active_tetramino_.rot_index + 1) % 4;
  }

  return this;
}

void GameScreen::draw() {
  fill_scrfeen_buffer(1);
  draw_tetramino(active_tetramino_);
  draw_tetramino(sliding_tetramino_);
  draw_trajectory();
  draw_boundaries();
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
  planet_state_.distance.value = 1.496E11f;
  planet_state_.angle.speed = 1.990986E-7f;
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

void GameScreen::update_sliding_tetramino(ActiveTetramino& block) {
  if (block.block == NULL) {
    return;
  }

  if (block.progress < 1) {
    block.progress += PROGRESS_SPEED;
    block.pos = vector2_lerp(block.oldPos, block.targetPos, block.progress);
    trace("Sliding to (%.1f; %.1f)\n", block.pos.x, block.pos.y);
    return;
  }

  trace("Arrived at (%.1f %.1f), check next\n", block.targetPos.x, block.targetPos.y);

  int threshold = TILE_W;
  float dx = CENTER_X - block.pos.x;
  float dy = CENTER_Y - block.pos.y;

  int dirX = dx > 0 ? 1 : -1;
  int dirY = dy > 0 ? 1 : -1;

  if (abs(dx) < threshold) {
    dirX = 0;
  }

  if (abs(dy) < threshold) {
    dirY = 0;
  }

  if (dirX == 0 && dirY == 0) {
    trace("We're here!\n");
    tilemap_.place_tetramino(block);
    block.block = NULL;
    return;
  }

  // check larger distance first
  int move = 0;
  if (abs(dx) > abs(dy)) {
    if (dirX != 0 && tilemap_.can_move(block, dirX, 0)) {
      printf("Can move %d on x\n", dirX);
      dirY = 0;
      move = 1;
    } else if (dirY != 0 && tilemap_.can_move(block, 0, dirY)) {
      printf("Can move %d on y\n", dirY);
      dirX = 0;
      move = 1;
    }
  } else {
    if (dirY != 0 && tilemap_.can_move(block, 0, dirY)) {
      printf("Can move %d on y\n", dirY);
      dirX = 0;
      move = 1;
    } else if (dirX != 0 && tilemap_.can_move(block, dirX, 0)) {
      printf("Can move %d on x\n", dirX);
      dirY = 0;
      move = 1;
    }
  }

  if (!move) {
    trace("Can't move further!\n");
    tilemap_.place_tetramino(block);
    block.block = NULL;
    return;
  }

  int ix = 0, iy = 0;
  tilemap_.get_tetramino_tilemap_pos_corner(block, ix, iy);
  Vector2 newCornerPos = tilemap_.get_tile_pos(ix + dirX, iy + dirY);
  Vector2 newCenterPos = vector2_add(newCornerPos, vector2_scale(block.block->center, TILE_W));
  block.targetPos = newCenterPos;
  trace("Move to (%f %f)\n", newCenterPos.x, newCenterPos.y);
  block.oldPos = block.pos;
  block.progress = 0.0f;
}

void GameScreen::draw_trajectory() {
  OrbitalElements elements = calc_orbital_elements(planet_state_, STAR_MASS);
  if (elements.eccentricity >= 1.0f) {
    // TODO: draw open orbit!
    return;
  }

  // Calculate the step size for the true anomaly
  const float d_nu = 2.0f * (float)M_PI / (float)trajectory_size;

  Vector2 cur{};
  Vector2 prev{};

  int pattern_state = 0;
  for (int i = 0; i <= trajectory_size; ++i) {
    // Start drawing from the current planet position
    const float nu = planet_state_.angle.value - elements.arg_periapsis + i * d_nu;

    // Calculate Distance (r) using the Polar Equation of a Conic Section
    // r(nu) = p / (1 + e * cos(nu))
    float cos_nu = cosf(nu);
    float r_distance = elements.semi_latus_rectum / (1.0f + elements.eccentricity * cos_nu);

    // Calculate the Absolute Angle (phi)
    // phi = omega + nu
    const float phi = elements.arg_periapsis + nu;

    // Convert Polar coordinates to Cartesian
    float cos_phi = cosf(phi);
    float sin_phi = sinf(phi);

    // Scale the distance and offset center
    float dist_to_scale = r_distance / DIST_SCALE;
    cur.x = star_pos_.x + dist_to_scale * cos_phi;
    cur.y = star_pos_.y + dist_to_scale * sin_phi;

    // Draw the Line Segment
    if (i > 0) {
      size_t pattern_index = (size_t)remap(i, 0, trajectory_size, 0, patterns_count - 1);
      pattern_state = draw_line_pattern((int)prev.x, (int)prev.y, (int)cur.x, (int)cur.y,
                                        pattern_state, pattern_sizes[pattern_index], patterns[pattern_index]);
    }

    prev = cur;
  }
}

void GameScreen::draw_boundaries() {
  // draw tetirs boundaries
  Rectangle rect{
    0.0f, 0.0f,
    ROW_LENGTH * TILE_W,
    ROW_LENGTH * TILE_H
  };

  rect.x = CENTER_X - rect.width / 2;
  rect.y = CENTER_Y - rect.height / 2;

  constexpr uint8_t pattern_50_percent = 0xAA;
  draw_rectangle_lines_pattern(rect, 8, pattern_50_percent);

  rect.width = DEATH_LENGTH * TILE_W;
  rect.height = DEATH_LENGTH * TILE_H;
  rect.x = CENTER_X - rect.width / 2;
  rect.y = CENTER_Y - rect.height / 2;

  constexpr uint8_t pattern_66_percent = 0xd8;
  draw_rectangle_lines_pattern(rect, 6, pattern_66_percent);
}
