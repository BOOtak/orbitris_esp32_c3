#include "button_grid_manager.h"

#include "const.h"
#include "draw.h"
#include "input.h"

constexpr int ANIMATION_FRAMES = 30;

ButtonGridManager::ButtonGridManager(
  Button *buttons, size_t count, int *custom_map, size_t rs, size_t cs)
  : all_buttons_(buttons),
    count_(count),
    grid_map_(custom_map),
    rows_(rs),
    cols_(cs),
    focused_grid_row_(BUTTON_NO_ACTION),
    focused_grid_col_(BUTTON_NO_ACTION),
    anim_timer_(0),
    action_was_pressed_(false) {
  init_focus();
}

void ButtonGridManager::init() {
  action_was_pressed_ = false;
  init_focus();
}

int ButtonGridManager::update() {
  int prev_r = focused_grid_row_;
  int prev_c = focused_grid_col_;

  int dir_r = 0;
  int dir_c = 0;
  bool input_detected = false;
  int action_id = BUTTON_NO_ACTION;  // Store potential action ID

  if (is_key_pressed(ESP_KEY_UP)) {
    dir_r = -1;
    input_detected = true;
  } else if (is_key_pressed(ESP_KEY_DOWN)) {
    dir_r = 1;
    input_detected = true;
  } else if (is_key_pressed(ESP_KEY_LEFT)) {
    dir_c = -1;
    input_detected = true;
  } else if (is_key_pressed(ESP_KEY_RIGHT)) {
    dir_c = 1;
    input_detected = true;
  }

  if (input_detected) {
    int prev_button_index = grid_map_[prev_r * cols_ + prev_c];
    int target_r = prev_r + dir_r;
    int target_c = prev_c + dir_c;

    while (target_r >= 0 && target_r < rows_ && target_c >= 0 && target_c < cols_) {
      int current_cell_index = grid_map_[target_r * cols_ + target_c];

      if (current_cell_index != BUTTON_NO_ACTION && current_cell_index != prev_button_index) {
        // Found a new, valid button!
        focused_grid_row_ = target_r;
        focused_grid_col_ = target_c;
        update_focus_state_logic(prev_button_index, current_cell_index);
        break;
      }

      target_r += dir_r;
      target_c += dir_c;
    }
  }

  if (is_key_pressed(ESP_KEY_A)) {
    action_was_pressed_ = true;
    int index = grid_map_[focused_grid_row_ * cols_ + focused_grid_col_];

    if (index != BUTTON_NO_ACTION && index < count_) {
      all_buttons_[index].set_state(ButtonState::pressed);
    }
  }

  if (is_key_released(ESP_KEY_A) && action_was_pressed_) {
    int index = grid_map_[focused_grid_row_ * cols_ + focused_grid_col_];

    // Store the ID to be returned
    action_id = all_buttons_[index].get_id();

    // Revert to focused state
    all_buttons_[index].set_state(ButtonState::focused);
  }

  if (anim_timer_ < ANIMATION_FRAMES) {
    anim_timer_++;
  }

  return action_id;
}

void ButtonGridManager::draw() const {
  for (size_t i = 0; i < count_; i++) {
    all_buttons_[i].draw();
  }

  draw_animated_focus_frame();
}

void ButtonGridManager::init_focus() {
  int prev_r = focused_grid_row_;
  int prev_c = focused_grid_col_;
  int prev_button_index = BUTTON_NO_ACTION;
  if (prev_c != BUTTON_NO_ACTION && prev_r != BUTTON_NO_ACTION) {
    prev_button_index = grid_map_[prev_r * cols_ + prev_c];
  }

  // Set initial focus to the first valid button in the map
  for (int r = 0; r < rows_; ++r) {
    for (int c = 0; c < cols_; ++c) {
      if (grid_map_[r * cols_ + c] != BUTTON_NO_ACTION) {
        focused_grid_row_ = r;
        focused_grid_col_ = c;

        int initial_index = grid_map_[r * cols_ + c];
        anim_target_rect_ = get_animation_rect(initial_index);
        current_focus_rect_ = anim_target_rect_;
        anim_start_rect_ = current_focus_rect_;
        anim_timer_ = ANIMATION_FRAMES;

        update_focus_state_logic(prev_button_index, initial_index);
        return;
      }
    }
  }
}

void ButtonGridManager::update_focus_state_logic(int old_index, int new_index) {
  // 1. Update visual state of old button
  if (old_index != BUTTON_NO_ACTION && old_index < count_) {
    all_buttons_[old_index].set_state(ButtonState::idle);
  }

  // 2. Update visual state of new button
  if (new_index != BUTTON_NO_ACTION && new_index < count_) {
    all_buttons_[new_index].set_state(ButtonState::focused);
  }

  // 3. Animation Setup
  anim_start_rect_ = current_focus_rect_;
  anim_target_rect_ = get_animation_rect(new_index);
  anim_timer_ = 0;
}

void ButtonGridManager::draw_animated_focus_frame() const {
  if (anim_timer_ < ANIMATION_FRAMES) {
    float t_normalized = (float)anim_timer_ / ANIMATION_FRAMES;
    float t_eased = ease_out_quad(t_normalized);

    // Interpolate and store the result
    current_focus_rect_.x = my_lerp(anim_start_rect_.x, anim_target_rect_.x, t_eased);
    current_focus_rect_.y = my_lerp(anim_start_rect_.y, anim_target_rect_.y, t_eased);
    current_focus_rect_.width = my_lerp(anim_start_rect_.width, anim_target_rect_.width, t_eased);
    current_focus_rect_.height = my_lerp(anim_start_rect_.height, anim_target_rect_.height, t_eased);
  } else {
    current_focus_rect_.x = anim_target_rect_.x;
    current_focus_rect_.y = anim_target_rect_.y;
    current_focus_rect_.width = anim_target_rect_.width;
    current_focus_rect_.height = anim_target_rect_.height;
  }

  // Draw the frame
  int x = current_focus_rect_.x;
  int y = current_focus_rect_.y;
  int w = current_focus_rect_.width;
  int h = current_focus_rect_.height;

  draw_rectangle_lines(x, y, w, h, LCD_BLACK);
}

Rectangle ButtonGridManager::get_animation_rect(int index) const {
  constexpr int offset = 2;
  const Rectangle &r = all_buttons_[index].get_rect();
  return { r.x - offset, r.y - offset, r.width + offset * 2, r.height + offset * 2 };
}
