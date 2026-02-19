#include "game_screen.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "draw.h"
#include "explosion.h"
#include "input.h"
#include "table_math.h"
#include "trace.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr float DIST_SCALE = 6.5E8f * 2;
constexpr float STAR_MASS = 1.98855E30f;

constexpr int resolution_min = 1;  // times per frame
constexpr int resolution_max = 10;
constexpr int d_resolution = resolution_max - resolution_min;

constexpr float dist_max = 1.496E11f;
constexpr float dist_min = 3.5E8f;
constexpr float d_dist = dist_max - dist_min;

constexpr float SCALE_MIN_DIST = LCD_HEIGHT * 1.5f;
constexpr float SCALE_MAX_DIST = LCD_HEIGHT / 2.0f;

constexpr float SCALE_MIN = 0.33f;
constexpr float SCALE_MAX = 1.0f;
constexpr float ZOOM_SPEED = 0.005f;

constexpr auto trajectory_size = 50;

constexpr float PROGRESS_SPEED = 0.05f;
constexpr float DIST_THRESHOLD = 0.0001f;

constexpr float MAX_PIECE_DISTANCE_SQUARE = LCD_HEIGHT * LCD_HEIGHT * 4;

constexpr Vector2 NEXT_TETRAMINO_POS = { 80, 20 };

const uint8_t patterns[] = { 0x00, 0x11, 0x24, 0x55, 0xd8, 0xee, 0xf0, 0xf8 };
const uint8_t pattern_sizes[] = { 8, 8, 6, 8, 6, 8, 5, 6 };
constexpr auto patterns_count = std::size(patterns);

constexpr int GAME_OVER_ANIMATION_FRAMES = 60;

const char* status_text_heres_your_piece = "Here's your piece\nDon't lose it again!";

GameScreen::GameScreen(Stats& stats)
  : Screen(), stats_{ stats } {}

void GameScreen::init() {
  stats_ = {};

  active_tetramino_ = {};
  next_tetramino_ = { 0, NEXT_TETRAMINO_POS, nullptr };
  sliding_tetramino_ = {};
  is_exploding_ = false;
  is_playing_game_over_animation_ = false;
  game_over_animation_frame_ = 0;

  generate_next_tetramino();

  reset_planet_state();
  delta_time_ = 3600 * 24;

  current_zoom_ = 1.0f;
  target_zoom_ = 1.0f;

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
    } else if (vector2_square_dist(active_tetramino_.pos, star_pos_) < ((TILE_W * TILE_W) / 4)) {
      trace("Found new center!\n");
      tilemap_.place_tetramino(active_tetramino_);
      reset_planet_state();
      generate_next_tetramino();
      active_tetramino_.pos = state_to_coords(planet_state_, DIST_SCALE, star_pos_);
      break;
    }
  }

  if (collision.width > 0 && collision.height > 0) {
    sliding_tetramino_ = active_tetramino_;
    sliding_tetramino_.progress = 1.1f;  // to check where to slide it first
    reset_planet_state();
    generate_next_tetramino();
    active_tetramino_.pos = state_to_coords(planet_state_, DIST_SCALE, star_pos_);
  }

  if (tilemap_.tile_out_of_bounds && !is_playing_game_over_animation_) {
    is_playing_game_over_animation_ = true;
  }

  if (is_playing_game_over_animation_) {
    game_over_animation_frame_++;
    if (game_over_animation_frame_ >= GAME_OVER_ANIMATION_FRAMES && !is_exploding_) {
      init_explosion(tilemap_, star_pos_);
      is_exploding_ = true;
    }
  }

  update_sliding_tetramino(sliding_tetramino_);

  detect_piece_too_far(active_tetramino_);

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

  if (is_key_pressed(ESP_KEY_A)) {
    return screens::game_over_screen;
  }

  if (is_key_pressed(ESP_KEY_B)) {
    is_playing_game_over_animation_ = true;
  }

  if (is_exploding_) {
    if (update_explosion()) {
      return screens::game_over_screen;
    }
  }

  // update stats
  stats_.game_points = tilemap_.game_points;

  // prepare for draw
  float diff = current_zoom_ - target_zoom_;
  if (fabs(diff) > ZOOM_SPEED) {
    int dir = diff < 0 ? 1 : -1;
    current_zoom_ += ZOOM_SPEED * dir;
  }

  float apoapsis = calc_apoapsis(planet_state_, STAR_MASS);
  if (apoapsis > DIST_THRESHOLD) {
    float scaled_apoapsis = apoapsis / DIST_SCALE;
    target_zoom_ = remap(std::clamp(scaled_apoapsis, SCALE_MAX_DIST, SCALE_MIN_DIST),
                         SCALE_MAX_DIST, SCALE_MIN_DIST, SCALE_MAX, SCALE_MIN);
  } else {
    target_zoom_ = 1.0f;
  }

  if (status_text_frame_ < STATUS_TEXT_FRAMES) {
    status_text_frame_++;
  }

  return this;
}

void GameScreen::draw() const {
  fill_scrfeen_buffer(1);

  begin_scale(current_zoom_);
  draw_trajectory();
  draw_boundaries();
  draw_tetramino(active_tetramino_);
  draw_tetramino(sliding_tetramino_);
  if (is_exploding_) {
    draw_explosion();
  } else {
    tilemap_.draw();
  }
  end_scale();

  constexpr auto buf_size = 100;
  char score_buf[buf_size]{};
  snprintf(score_buf, buf_size, "Score: %d\n", tilemap_.game_points);
  print_text(280, 10, 2, score_buf, 0);
  print_text(10, 10, 2, "Next:", 0);

  if (status_text_frame_ < STATUS_TEXT_FRAMES) {
    constexpr int text_y_offset = 10;
    Vector2 text_size = measure_text(status_text_, 2);
    print_text(LCD_WIDTH / 2 - text_size.x / 2,
               LCD_HEIGHT - text_size.y - text_y_offset, 2, status_text_, 0);
  }
  draw_tetramino(next_tetramino_);
}

void GameScreen::close() {
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
  Vector2 newCenterPos = newCornerPos + block.block->center * TILE_W;
  block.targetPos = newCenterPos;
  trace("Move to (%f %f)\n", newCenterPos.x, newCenterPos.y);
  block.oldPos = block.pos;
  block.progress = 0.0f;
}

void GameScreen::detect_piece_too_far(const ActiveTetramino& block) {
  if (vector2_square_dist(block.pos, star_pos_) > MAX_PIECE_DISTANCE_SQUARE) {
    reset_planet_state();
    status_text_frame_ = 0;
    status_text_ = status_text_heres_your_piece;
  }
}

void GameScreen::draw_trajectory() const {
  OrbitalElements elements = calc_orbital_elements(planet_state_, STAR_MASS);
  if (elements.eccentricity >= 1.0f) {
    // TODO: draw open orbit!
    return;
  }

  // Calculate the step size for the true anomaly
  int dir = planet_state_.angle.speed > 0 ? 1 : -1;
  const float d_nu = 2.0f * (float)M_PI / (float)trajectory_size * dir;

  Vector2 cur{};
  Vector2 prev{};

  int pattern_state = 0;
  for (int i = 0; i <= trajectory_size; ++i) {
    // Start drawing from the current planet position
    const float nu = planet_state_.angle.value - elements.arg_periapsis + i * d_nu;

    // Calculate Distance (r) using the Polar Equation of a Conic Section
    // r(nu) = p / (1 + e * cos(nu))
    float cos_nu = approx_cos(nu);
    float r_distance = elements.semi_latus_rectum / (1.0f + elements.eccentricity * cos_nu);

    // Calculate the Absolute Angle (phi)
    // phi = omega + nu
    const float phi = elements.arg_periapsis + nu;

    // Convert Polar coordinates to Cartesian
    float cos_phi = approx_cos(phi);
    float sin_phi = approx_sin(phi);

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

void GameScreen::draw_boundaries() const {
  float rect_scale = ease_out_cubic(remap(game_over_animation_frame_, 0, GAME_OVER_ANIMATION_FRAMES, 1.0f, 0.0f));

  // draw tetirs boundaries
  Rectangle rect{
    0.0f, 0.0f,
    ROW_LENGTH * TILE_W * rect_scale,
    ROW_LENGTH * TILE_H * rect_scale
  };

  rect.x = CENTER_X - rect.width / 2;
  rect.y = CENTER_Y - rect.height / 2;

  constexpr uint8_t pattern_50_percent = 0xAA;
  draw_rectangle_lines_pattern(rect, 8, pattern_50_percent);

  rect.width = DEATH_LENGTH * TILE_W * rect_scale;
  rect.height = DEATH_LENGTH * TILE_H * rect_scale;
  rect.x = CENTER_X - rect.width / 2;
  rect.y = CENTER_Y - rect.height / 2;

  constexpr uint8_t pattern_66_percent = 0xd8;
  draw_rectangle_lines_pattern(rect, 6, pattern_66_percent);
}
