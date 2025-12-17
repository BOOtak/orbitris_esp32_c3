#include "game_main.h"

#include "draw.h"
#include "game_screen.h"
#include "game_over_screen.h"
#include "game_utils.h"
#include "screen.h"
#include "stats.h"
#include "table_math.h"

constexpr float TRANSITION_SPEED = 1 / 25.0f;

constexpr DrawMask MASKS[] = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                               { 0x88, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00 },
                               { 0x22, 0x00, 0x88, 0x00, 0x22, 0x00, 0x88, 0x00 },
                               { 0x22, 0x00, 0xAA, 0x00, 0x22, 0x00, 0xAA, 0x00 },
                               { 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00 },
                               { 0xAA, 0x44, 0xAA, 0x00, 0xAA, 0x44, 0xAA, 0x00 },
                               { 0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88 },
                               { 0x55, 0x22, 0x55, 0xAA, 0x55, 0x22, 0x55, 0xAA },
                               { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA },
                               { 0x55, 0xBB, 0x55, 0xAA, 0x55, 0xBB, 0x55, 0xAA },
                               { 0x55, 0xBB, 0x55, 0xEE, 0x55, 0xBB, 0x55, 0xEE },
                               { 0x55, 0xBB, 0x55, 0xFF, 0x55, 0xBB, 0x55, 0xFF },
                               { 0x55, 0xFF, 0x55, 0xFF, 0x55, 0xFF, 0x55, 0xFF },
                               { 0x77, 0xFF, 0x55, 0xFF, 0x77, 0xFF, 0x55, 0xFF },
                               { 0x77, 0xFF, 0xDD, 0xFF, 0x77, 0xFF, 0xDD, 0xFF },
                               { 0x77, 0xFF, 0xFF, 0xFF, 0x77, 0xFF, 0xFF, 0xFF },
                               { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };

constexpr int MASKS_COUNT = sizeof(MASKS) / sizeof(MASKS[0]);

Screen* current_screen = nullptr;
Screen* next_screen = nullptr;

bool in_transition = false;
float transition_progress = 0.0f;

Stats stats{};

float ease_out_cubic(float x) {
  return 1 - pow(1 - x, 3);
}

float ease_out_circ(float x) {
  return sqrt(1 - (x - 1) * (x - 1));
}

void start_transition(Screen* from, Screen* to) {
  next_screen = to;
  in_transition = true;
  transition_progress = 0.0f;

  // Init new state when transition begins
  next_screen->init();
}

void update_transition() {
  transition_progress += TRANSITION_SPEED;

  next_screen->update();

  if (transition_progress >= 1.0f) {
    // Called only when transition ends
    current_screen->close();
    current_screen = next_screen;
    next_screen = nullptr;
    in_transition = false;
  }
}

void draw_transition() {
  constexpr float zoom_end_old = 3.0f;
  constexpr float zoom_start_new = 0.2f;

  float ease_progress = ease_out_cubic(transition_progress);
  float zoom_old = lerp(1.0f, zoom_end_old, ease_progress);
  float zoom_new = lerp(zoom_start_new, 1.0f, ease_progress);

  int mask_index = (int)remap(ease_progress, 0.0f, 1.0f, 0.0f, MASKS_COUNT - 1);
  DrawMask new_mask = MASKS[mask_index];
  DrawMask old_mask = ~new_mask;

  begin_screen_scale(zoom_old);
  begin_mask(old_mask);
  current_screen->draw();
  end_mask();
  end_screen_scale();
  begin_screen_scale(zoom_new);
  begin_mask(new_mask);
  next_screen->draw();
  end_mask();
  end_screen_scale();
}

void update_screen() {
  Screen* new_screen = current_screen->update();
  if (new_screen != current_screen) {
    start_transition(current_screen, new_screen);
  }
}

void draw_screen() {
  current_screen->draw();
}

void init_game() {
  init_trig_tables();
  screens::game_screen = new GameScreen(stats);
  screens::game_over_screen = new GameOverScreen(stats);
  current_screen = screens::game_screen;
  current_screen->init();
  in_transition = false;
}

void update_draw_frame() {
  if (in_transition) {
    update_transition();
  } else {
    update_screen();
  }

  if (in_transition) {
    draw_transition();
  } else {
    draw_screen();
  }
}
