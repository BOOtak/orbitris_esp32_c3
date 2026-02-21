#pragma once

#include <cstddef>

#include "button.h"

constexpr int BUTTON_NO_ACTION = -1;

class ButtonGridManager {
public:

  ButtonGridManager(Button* buttons, size_t count, int* custom_map, size_t rs, size_t cs);

  void init();

  /**
     * @brief Processes input, updates focus/animation, and returns the ID of the pressed button.
     * @return The ID of the button pressed with key A, or BUTTON_NO_ACTION (-1) otherwise.
     */
  int update();

  void draw() const;

private:
  Button* all_buttons_;
  size_t count_;

  int* grid_map_;
  size_t rows_;
  size_t cols_;

  int focused_grid_row_;
  int focused_grid_col_;

  mutable Rectangle current_focus_rect_;
  Rectangle anim_start_rect_;
  Rectangle anim_target_rect_;
  int anim_timer_;

  bool action_was_pressed_;

  void init_focus();

  void update_focus_state_logic(int old_index, int new_index);

  void draw_animated_focus_frame() const;

  Rectangle get_animation_rect(int index) const;
};
