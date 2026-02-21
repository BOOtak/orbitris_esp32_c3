#include "pause_screen.h"

#include <array>
#include <cstdio>

#include "input.h"
#include "const.h"
#include "draw.h"
#include "charmap.h"

constexpr auto TEXT_SCALE = 2;
constexpr auto TEXT_HEIGHT = FONT_CHAR_HEIGHT * TEXT_SCALE;

const char* label_continue = "Continue";
const char* label_main_menu = "Main menu";

constexpr int id_continue = 0;
constexpr int id_main_menu = 1;

constexpr int button_width = 150;
constexpr int button_height = 32;

constexpr int start_x = (LCD_WIDTH - button_width * 2) / 3;
constexpr int delta_x = 34 + button_width;
constexpr int start_y = 200;

PauseScreen::PauseScreen()
  : Screen(),
    pause_buttons_{
      { { start_x + delta_x * 0, start_y, button_width, button_height }, label_continue, 2, id_continue },
      { { start_x + delta_x * 1, start_y, button_width, button_height }, label_main_menu, 2, id_main_menu }
    },
    grid_map_{ id_continue, id_main_menu },
    manager_{ pause_buttons_, std::size(pause_buttons_), grid_map_, 1, 2 } {
  snprintf(text_buffer_, std::size(text_buffer_), "Pause");
  text_size_ = measure_text(text_buffer_, TEXT_SCALE);
}

void PauseScreen::init() {
  manager_.init();
}

Screen* PauseScreen::update() {
  auto id = manager_.update();
  switch (id) {
    case id_continue:
      return screens::game_screen;
    case id_main_menu:
      return screens::menu_screen;
    default:
      break;
  }

  return this;
}

void PauseScreen::draw() const {
  draw_rectangle_checkerboard(0, 0, LCD_WIDTH, LCD_HEIGHT);

  const int text_x = LCD_WIDTH / 2 - text_size_.x / 2;
  const int text_y = LCD_HEIGHT / 2 - text_size_.y / 2;

  draw_rectangle(Rectangle{ text_x - 2.0f, text_y - 2.0f, text_size_.x + 4, text_size_.y + 4 }, LCD_WHITE);
  print_text(text_x, text_y, 2, text_buffer_, 0);

  draw_rectangle(Rectangle{0.0f, start_y - 5.0f, LCD_WIDTH, 45.0f}, LCD_WHITE);

  manager_.draw();
}
