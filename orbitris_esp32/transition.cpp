#include "transition.h"

#include <math.h>

#include "draw.h"

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
                               { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
                               { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };

constexpr int MASKS_COUNT = sizeof(MASKS) / sizeof(MASKS[0]);

static void draw_tarnsition_zoom_out();

static void draw_tarnsition_zoom_in();

static Screen* current_screen = nullptr;
static Screen* next_screen = nullptr;

static float transition_progress = 0.0f;
static TransitionParams transition_params;

void (*draw_transition)();

float ease_out_cubic(float x) {
  return 1 - pow(1 - x, 3);
}

float ease_out_circ(float x) {
  return sqrt(1 - (x - 1) * (x - 1));
}

void start_transition(Screen* from, Screen* to, TransitionParams params) {
  current_screen = from;
  next_screen = to;
  transition_progress = 0.0f;
  transition_params = params;

  // Init new state when transition begins
  next_screen->init();

  switch (params.kind) {
    case TransitionKind::ZOOM_IN:
      draw_transition = draw_tarnsition_zoom_in;
      break;
    case TransitionKind::ZOOM_OUT:
      draw_transition = draw_tarnsition_zoom_out;
      break;
    default:
      break;
  }
}

bool update_transition() {
  transition_progress += transition_params.speed;

  next_screen->update();

  if (transition_progress >= 1.0f) {
    // Called only when transition ends
    current_screen->close();
    current_screen = next_screen;
    next_screen = nullptr;
    return true;
  }

  return false;
}

static void draw_tarnsition_zoom_in() {
  constexpr float zoom_end_old = 3.0f;
  constexpr float zoom_start_new = 0.2f;

  float ease_progress = ease_out_cubic(transition_progress);
  float zoom_old = my_lerp(1.0f, zoom_end_old, ease_progress);
  float zoom_new = my_lerp(zoom_start_new, 1.0f, ease_progress);

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

static void draw_tarnsition_zoom_out() {
  constexpr float zoom_end_old = 0.2f;
  constexpr float zoom_start_new = 3.0f;

  float ease_progress = ease_out_cubic(transition_progress);
  float zoom_old = my_lerp(1.0f, zoom_end_old, ease_progress);
  float zoom_new = my_lerp(zoom_start_new, 1.0f, ease_progress);

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