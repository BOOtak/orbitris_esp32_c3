#include "highscore_input_screen.h"

#include "const.h"
#include "draw.h"

constexpr auto BUTTON_COLUMNS = 10;
constexpr auto BUTTON_ROWS = 4;

constexpr auto button_width = 32;
constexpr auto button_height = 32;

constexpr auto button_horizontal_total_padding = LCD_WIDTH - (button_width * BUTTON_COLUMNS);
constexpr auto buttons_distance = button_horizontal_total_padding / (BUTTON_COLUMNS);
constexpr auto start_x = buttons_distance / 2;

constexpr auto end_y = LCD_HEIGHT - buttons_distance / 2;
constexpr auto keyboard_height = BUTTON_ROWS * button_height + (buttons_distance * (BUTTON_ROWS - 1));
constexpr auto start_y = end_y - keyboard_height;

constexpr auto dx = button_width + buttons_distance;
constexpr auto dy = button_height + buttons_distance;

constexpr int ID_ENTER = 29;
constexpr int ID_LEFT = 37;
constexpr int ID_RIGHT = 38;

constexpr int char_indices[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ID_ENTER,
  'Z', 'X', 'C', 'V', 'B', 'N', 'M', ID_LEFT, ID_RIGHT, ' '
};

HighscoreInputScreen::HighscoreInputScreen()
  : Screen(), keyboard_{
      { { start_x + dx * 0, start_y + dy * 0, button_width, button_height }, "0", 2, 0 },
      { { start_x + dx * 1, start_y + dy * 0, button_width, button_height }, "1", 2, 1 },
      { { start_x + dx * 2, start_y + dy * 0, button_width, button_height }, "2", 2, 2 },
      { { start_x + dx * 3, start_y + dy * 0, button_width, button_height }, "3", 2, 3 },
      { { start_x + dx * 4, start_y + dy * 0, button_width, button_height }, "4", 2, 4 },
      { { start_x + dx * 5, start_y + dy * 0, button_width, button_height }, "5", 2, 5 },
      { { start_x + dx * 6, start_y + dy * 0, button_width, button_height }, "6", 2, 6 },
      { { start_x + dx * 7, start_y + dy * 0, button_width, button_height }, "7", 2, 7 },
      { { start_x + dx * 8, start_y + dy * 0, button_width, button_height }, "8", 2, 8 },
      { { start_x + dx * 9, start_y + dy * 0, button_width, button_height }, "9", 2, 9 },

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
    manager_{ keyboard_, KEYS_COUNT, nullptr, BUTTON_ROWS, BUTTON_COLUMNS } {
}

void HighscoreInputScreen::init() {
}

Screen *HighscoreInputScreen::update() {
  manager_.update();
  return this;
}

void HighscoreInputScreen::draw() const {
  fill_scrfeen_buffer(LCD_WHITE);
  manager_.draw();
}
