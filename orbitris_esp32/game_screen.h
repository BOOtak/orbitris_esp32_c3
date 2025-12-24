#pragma once

#include <cstdint>

#include "const.h"
#include "game_utils.h"
#include "orbital.h"
#include "screen.h"
#include "stats.h"
#include "tetramino.h"
#include "tilemap.h"

constexpr int STATUS_TEXT_FRAMES = 120;

class GameScreen : public Screen {
public:
  GameScreen(Stats& stats);

  virtual void init() override;

  virtual Screen* update() override;

  virtual void draw() const override;

  virtual void close() override;

private:
  Stats& stats_;
  Tilemap tilemap_{};
  ActiveTetramino active_tetramino_;
  ActiveTetramino next_tetramino_;
  ActiveTetramino sliding_tetramino_;
  PlanetState planet_state_;
  Vector2 star_pos_{ LCD_WIDTH / 2, LCD_HEIGHT / 2 };
  float delta_time_;
  float current_zoom_;
  float target_zoom_;
  int status_text_frame_{ STATUS_TEXT_FRAMES };
  const char* status_text_{};

  int get_resolution(const PlanetState& planet);

  void reset_planet_state();
  void generate_next_tetramino();
  void update_sliding_tetramino(ActiveTetramino& block);
  void detect_piece_too_far(const ActiveTetramino& block);

  void draw_trajectory() const;
  void draw_boundaries() const;
};
