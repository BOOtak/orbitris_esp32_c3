#include "authors_screen.h"

#include "authors_bitmap.h"
#include "const.h"
#include "draw.h"
#include "input.h"

AuthorsScreen::AuthorsScreen() {
}

void AuthorsScreen::init() {
}

Screen* AuthorsScreen::update() {
  if (is_key_pressed(ESP_KEY_A)) {
    return screens::menu_screen;
  }

  return this;
}

void AuthorsScreen::draw() const {
  fill_screen_buffer(LCD_WHITE);
  draw_bitmap(authors_bitmap_data);
  print_text(160, 32, 2, "These guys!", LCD_BLACK);
}
