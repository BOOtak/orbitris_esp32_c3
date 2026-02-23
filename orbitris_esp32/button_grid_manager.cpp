#include "button_grid_manager.h"

#include "const.h"
#include "draw.h"
#include "input.h"

constexpr int ANIMATION_FRAMES = 30;

// Repeat timing constants (in frames)
constexpr int REPEAT_DELAY_FRAMES = 18;    // frames before first repeat (e.g., ~300ms at 60fps)
constexpr int REPEAT_INTERVAL_FRAMES = 6;  // frames between subsequent repeats (e.g., ~100ms at 60fps)

struct DirectionInfo {
  int key;
  int dr;
  int dc;
};

constexpr DirectionInfo directions[DIR_COUNT] = {
  { ESP_KEY_UP, -1, 0 },
  { ESP_KEY_DOWN, 1, 0 },
  { ESP_KEY_LEFT, 0, -1 },
  { ESP_KEY_RIGHT, 0, 1 }
};

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
    action_was_pressed_(false),
    repeat_state_{},
    repeat_timer_{} {
  init_focus();
}


void ButtonGridManager::init() {
  action_was_pressed_ = false;

  for (int i = 0; i < DIR_COUNT; ++i) {
    repeat_state_[i] = RepeatState::IDLE;
    repeat_timer_[i] = 0;
  }

  init_focus();
}

bool ButtonGridManager::find_next_valid_cell(int start_r, int start_c, int dr, int dc,
                                             int &out_r, int &out_c) const {
  int r = start_r;
  int c = start_c;

  while (true) {
    // Move one step with wrap
    r = (r + dr + rows_) % rows_;
    c = (c + dc + cols_) % cols_;

    // If we returned to the start, no other valid cell exists
    if (r == start_r && c == start_c) {
      return false;
    }

    int index = get_index(r * cols_ + c);
    if (index != BUTTON_NO_ACTION) {
      out_r = r;
      out_c = c;
      return true;
    }
  }
}

int ButtonGridManager::update() {
  int prev_r = focused_grid_row_;
  int prev_c = focused_grid_col_;
  int prev_index = get_index(prev_r * cols_ + prev_c);

  int action_id = BUTTON_NO_ACTION;  // Store potential action ID

  // Process each direction until we find a movement
  for (int dir = 0; dir < DIR_COUNT; ++dir) {
    const auto &info = directions[dir];

    if (is_key_down(info.key)) {
      // Determine if we should move based on repeat state
      bool should_move = false;

      if (repeat_state_[dir] == RepeatState::IDLE) {
        // First press - move immediately
        should_move = true;
        repeat_state_[dir] = RepeatState::DELAY;
        repeat_timer_[dir] = 0;
      } else {
        // Already in delay or repeat state - check timer
        repeat_timer_[dir]++;

        int threshold = (repeat_state_[dir] == RepeatState::DELAY) ? REPEAT_DELAY_FRAMES : REPEAT_INTERVAL_FRAMES;

        if (repeat_timer_[dir] >= threshold) {
          should_move = true;
          repeat_state_[dir] = RepeatState::REPEAT;
          repeat_timer_[dir] = 0;
        }
      }

      // Perform movement if needed
      if (should_move) {
        int new_r, new_c;
        if (find_next_valid_cell(prev_r, prev_c, info.dr, info.dc, new_r, new_c)) {
          focused_grid_row_ = new_r;
          focused_grid_col_ = new_c;
          int new_index = get_index(new_r * cols_ + new_c);
          update_focus_state_logic(prev_index, new_index);
          break;
        }
      }
    } else {
      // Key released - reset state
      repeat_state_[dir] = RepeatState::IDLE;
      repeat_timer_[dir] = 0;
    }
  }

  if (is_key_pressed(ESP_KEY_A)) {
    action_was_pressed_ = true;
    int index = get_index(focused_grid_row_ * cols_ + focused_grid_col_);

    if (index != BUTTON_NO_ACTION && index < count_) {
      all_buttons_[index].state = ButtonState::pressed;
    }
  }

  if (is_key_released(ESP_KEY_A) && action_was_pressed_) {
    int index = get_index(focused_grid_row_ * cols_ + focused_grid_col_);

    // Store the ID to be returned
    action_id = all_buttons_[index].id;

    // Revert to focused state
    all_buttons_[index].state = ButtonState::focused;
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
    prev_button_index = get_index(prev_r * cols_ + prev_c);
  }

  // Set initial focus to the first valid button in the map
  for (int r = 0; r < rows_; ++r) {
    for (int c = 0; c < cols_; ++c) {
      if (get_index(r * cols_ + c) != BUTTON_NO_ACTION) {
        focused_grid_row_ = r;
        focused_grid_col_ = c;

        int initial_index = get_index(r * cols_ + c);
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
    all_buttons_[old_index].state = ButtonState::idle;
  }

  // 2. Update visual state of new button
  if (new_index != BUTTON_NO_ACTION && new_index < count_) {
    all_buttons_[new_index].state = ButtonState::focused;
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

int ButtonGridManager::get_index(int idx) const {
  if (grid_map_) {
    return grid_map_[idx];
  } else {
    return idx;
  }
}

Rectangle ButtonGridManager::get_animation_rect(int index) const {
  constexpr int offset = 2;
  const Rectangle &r = all_buttons_[index].rect;
  return { r.x - offset, r.y - offset, r.width + offset * 2, r.height + offset * 2 };
}
