#include "menu_screen.h"

#include <array>

#include "const.h"
#include "draw.h"
#include "trace.h"
#include "screen.h"

const char* label_new_game = "New game";
const char* label_settings = "Settings";
const char* label_authors = "Authors";

constexpr int id_new_game = 0;
constexpr int id_settings = 1;
constexpr int id_authors = 2;

constexpr int button_width = 150;
constexpr int button_height = 32;

constexpr int delta_y = 48;
constexpr int start_x = (LCD_WIDTH - button_width) / 2;
constexpr int start_y = (LCD_HEIGHT - button_height - (delta_y * (BUTTONS_COUNT - 1))) / 2;

MenuScreen::MenuScreen()
  : Screen(),
    menu_buttons_{
      { { start_x, start_y + delta_y * 0, button_width, button_height }, label_new_game, 2, id_new_game },
      { { start_x, start_y + delta_y * 1, button_width, button_height }, label_settings, 2, id_settings },
      { { start_x, start_y + delta_y * 2, button_width, button_height }, label_authors, 2, id_authors }
    },
    grid_map_{ id_new_game, id_settings, id_authors },
    manager_{ menu_buttons_, std::size(menu_buttons_), grid_map_, 3, 1 } {
}

void MenuScreen::init() {
}

Screen* MenuScreen::update() {
  int button_id = manager_.update();
  switch (button_id) {
    case id_new_game:
      return screens::game_screen;
      break;
    default:
      break;
  }
  return this;
}

void MenuScreen::draw() const {
  fill_scrfeen_buffer(LCD_WHITE);
  manager_.draw();
}
