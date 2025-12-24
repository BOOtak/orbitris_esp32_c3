#pragma once

#include "screen.h"

enum class TransitionKind {
  ZOOM_IN,
  ZOOM_OUT
};

void start_transition(Screen* from, Screen* to, TransitionKind kind);

/**
 * @brief Update transition state.
 * 
 * @return true if transition is complete, false otherwise
 */
bool update_transition();

extern void (*draw_transition)();
