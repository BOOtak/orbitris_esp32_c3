#include "highscore_input_screen.h"

#include "charmap.h"
#include "const.h"
#include "draw.h"
#include "input.h"
#include "persist.h"

#include <array>

constexpr auto BUTTON_COLUMNS = 10;
constexpr auto BUTTON_ROWS = 4;

constexpr auto button_width = 32;
constexpr auto button_height = 26;

constexpr auto button_horizontal_total_padding = LCD_WIDTH - (button_width * BUTTON_COLUMNS);
constexpr auto buttons_distance = button_horizontal_total_padding / (BUTTON_COLUMNS);
constexpr auto start_x = buttons_distance / 2;

constexpr auto end_y = LCD_HEIGHT - buttons_distance / 2 - FONT_CHAR_HEIGHT * 2 - 4;
constexpr auto keyboard_height = BUTTON_ROWS * button_height + (buttons_distance * (BUTTON_ROWS - 1));
constexpr auto start_y = end_y - keyboard_height;

constexpr auto dx = button_width + buttons_distance;
constexpr auto dy = button_height + buttons_distance;

constexpr int ID_ENTER = 29;
constexpr int ID_LEFT = 37;
constexpr int ID_RIGHT = 38;

constexpr int char_indices[] = {
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ID_ENTER,
  'Z', 'X', 'C', 'V', 'B', 'N', 'M', ID_LEFT, ID_RIGHT, ' '
};

HighscoreInputScreen::HighscoreInputScreen(const Stats &stats, HighscoreTable &highscores)
  : Screen(),
    stats_{ stats },
    highscores_{ highscores },
    keyboard_{
      { { start_x + dx * 0, start_y + dy * 0, button_width, button_height }, "1", 2, 0 },
      { { start_x + dx * 1, start_y + dy * 0, button_width, button_height }, "2", 2, 1 },
      { { start_x + dx * 2, start_y + dy * 0, button_width, button_height }, "3", 2, 2 },
      { { start_x + dx * 3, start_y + dy * 0, button_width, button_height }, "4", 2, 3 },
      { { start_x + dx * 4, start_y + dy * 0, button_width, button_height }, "5", 2, 4 },
      { { start_x + dx * 5, start_y + dy * 0, button_width, button_height }, "6", 2, 5 },
      { { start_x + dx * 6, start_y + dy * 0, button_width, button_height }, "7", 2, 6 },
      { { start_x + dx * 7, start_y + dy * 0, button_width, button_height }, "8", 2, 7 },
      { { start_x + dx * 8, start_y + dy * 0, button_width, button_height }, "9", 2, 8 },
      { { start_x + dx * 9, start_y + dy * 0, button_width, button_height }, "0", 2, 9 },

      { { start_x + dx * 0, start_y + dy * 1, button_width, button_height }, "Q", 2, 10 },
      { { start_x + dx * 1, start_y + dy * 1, button_width, button_height }, "W", 2, 11 },
      { { start_x + dx * 2, start_y + dy * 1, button_width, button_height }, "E", 2, 12 },
      { { start_x + dx * 3, start_y + dy * 1, button_width, button_height }, "R", 2, 13 },
      { { start_x + dx * 4, start_y + dy * 1, button_width, button_height }, "T", 2, 14 },
      { { start_x + dx * 5, start_y + dy * 1, button_width, button_height }, "Y", 2, 15 },
      { { start_x + dx * 6, start_y + dy * 1, button_width, button_height }, "U", 2, 16 },
      { { start_x + dx * 7, start_y + dy * 1, button_width, button_height }, "I", 2, 17 },
      { { start_x + dx * 8, start_y + dy * 1, button_width, button_height }, "O", 2, 18 },
      { { start_x + dx * 9, start_y + dy * 1, button_width, button_height }, "P", 2, 19 },

      { { start_x + dx * 0, start_y + dy * 2, button_width, button_height }, "A", 2, 20 },
      { { start_x + dx * 1, start_y + dy * 2, button_width, button_height }, "S", 2, 21 },
      { { start_x + dx * 2, start_y + dy * 2, button_width, button_height }, "D", 2, 22 },
      { { start_x + dx * 3, start_y + dy * 2, button_width, button_height }, "F", 2, 23 },
      { { start_x + dx * 4, start_y + dy * 2, button_width, button_height }, "G", 2, 24 },
      { { start_x + dx * 5, start_y + dy * 2, button_width, button_height }, "H", 2, 25 },
      { { start_x + dx * 6, start_y + dy * 2, button_width, button_height }, "J", 2, 26 },
      { { start_x + dx * 7, start_y + dy * 2, button_width, button_height }, "K", 2, 27 },
      { { start_x + dx * 8, start_y + dy * 2, button_width, button_height }, "L", 2, 28 },
      { { start_x + dx * 9, start_y + dy * 2, button_width, button_height }, "OK", 2, ID_ENTER },

      { { start_x + dx * 0, start_y + dy * 3, button_width, button_height }, "Z", 2, 30 },
      { { start_x + dx * 1, start_y + dy * 3, button_width, button_height }, "X", 2, 31 },
      { { start_x + dx * 2, start_y + dy * 3, button_width, button_height }, "C", 2, 32 },
      { { start_x + dx * 3, start_y + dy * 3, button_width, button_height }, "V", 2, 33 },
      { { start_x + dx * 4, start_y + dy * 3, button_width, button_height }, "B", 2, 34 },
      { { start_x + dx * 5, start_y + dy * 3, button_width, button_height }, "N", 2, 35 },
      { { start_x + dx * 6, start_y + dy * 3, button_width, button_height }, "M", 2, 36 },
      { { start_x + dx * 7, start_y + dy * 3, button_width, button_height }, "<", 2, ID_LEFT },
      { { start_x + dx * 8, start_y + dy * 3, button_width, button_height }, ">", 2, ID_RIGHT },
      { { start_x + dx * 9, start_y + dy * 3, button_width, button_height }, "__", 2, 39 }
    },
    manager_{ keyboard_, KEYS_COUNT, nullptr, BUTTON_ROWS, BUTTON_COLUMNS, true }, name_{}, cursor_position_{} {
}

void HighscoreInputScreen::init() {
  if (highscores_.last_name_index != HighscoreTable::NO_LAST_NAME_INDEX) {
    name_ = highscores_.names[highscores_.last_name_index];
  } else {
    std::strcpy(name_.data(), "FLOP");
  }
  cursor_position_ = std::strlen(name_.data());
}

Screen *HighscoreInputScreen::update() {
  int index = manager_.update();
  switch (index) {
    case ID_ENTER:
      if (name_[0] != '\0') {
        if (highscores_.add_highscore(name_.data(), stats_.game_points)) {
          persist_highscores(highscores_);
        }
        // TODO: return highscore table screen
        return screens::menu_screen;
      }
      break;
    case ID_LEFT:
      if (cursor_position_ > 0) {
        --cursor_position_;
      }
      break;
    case ID_RIGHT:
      if (cursor_position_ < HIGHSCORE_NAME_SIZE - 1) {
        if (name_[cursor_position_] == '\0') {
          name_[cursor_position_] = ' ';
        }
        cursor_position_++;
      }
      break;
    case BUTTON_NO_ACTION:
      break;
    default:
      {
        if (index < KEYS_COUNT) {
          auto sym = char_indices[index];
          if (cursor_position_ < HIGHSCORE_NAME_SIZE - 1) {
            name_[cursor_position_] = sym;
            cursor_position_++;
          }
        }
        break;
      }
  }

  // Backspace
  if (is_key_pressed(ESP_KEY_B)) {
    if (cursor_position_ > 0) {
      for (size_t i = cursor_position_; i < HIGHSCORE_NAME_SIZE; i++) {
        name_[i - 1] = name_[i];
      }

      --cursor_position_;
    }
  }

  return this;
}

void HighscoreInputScreen::draw() const {
  fill_screen_buffer(LCD_WHITE);

  constexpr auto prompt = "Name: ";
  constexpr auto prompt_size = measure_text(prompt, 2);

  constexpr auto text_size = measure_text("                ", 2);
  constexpr Vector2 name_start = { (LCD_WIDTH - 16 * FONT_CHAR_WIDTH * 2) / 2, (start_y - text_size.y) / 2 };
  print_text(name_start.x, name_start.y, 2, name_.data(), LCD_BLACK);
  print_text(name_start.x + cursor_position_ * FONT_CHAR_WIDTH * 2, name_start.y + 4, 2, "_", LCD_BLACK);

  constexpr auto prompt_start_x = name_start.x - prompt_size.x;
  print_text(prompt_start_x, name_start.y, 2, prompt, LCD_BLACK);

  constexpr Vector2 frame_margin = { 4.0f, 6.0f };
  constexpr auto frame_start = name_start - frame_margin;
  constexpr auto frame_size = text_size + frame_margin * 2;
  draw_rectangle_lines(frame_start.x, frame_start.y, frame_size.x, frame_size.y, LCD_BLACK);

  constexpr auto help_message = "   backspace";
  constexpr auto help_text_size = measure_text(help_message, 2);
  constexpr auto help_start_x = LCD_WIDTH - help_text_size.x - 4;
  constexpr auto help_start_y = LCD_HEIGHT - help_text_size.y - 4;

  print_text(help_start_x, help_start_y, 2, help_message, LCD_BLACK);
  print_text(help_start_x, help_start_y, 2, EXT_ENCIRCLED_BLACK_B, LCD_BLACK, true);
  manager_.draw();
}
