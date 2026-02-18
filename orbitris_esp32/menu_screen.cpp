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

MenuScreen::MenuScreen() : Screen(),
                           menu_buttons_{
                             { { 32, 32, 150, 32 }, label_new_game, 2, id_new_game },
                             { { 32, 80, 150, 32 }, label_settings, 2, id_settings },
                             { { 32, 128, 150, 32 }, label_authors, 2, id_authors }
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
