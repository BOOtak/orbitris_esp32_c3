#pragma once

#include <cstddef>

#include "screen.h"
#include "draw.h"

enum class TransitionKind {
  ZOOM_IN,
  ZOOM_OUT,
  DISSOLVE
};

struct TransitionParams {
  TransitionKind kind;
  float speed;
};

size_t get_masks_count();

DrawMask get_mask(size_t index);


void start_transition(Screen* from, Screen* to, TransitionParams params);

/**
 * @brief Update transition state.
 * 
 * @return true if transition is complete, false otherwise
 */
bool update_transition();

extern void (*draw_transition)();
