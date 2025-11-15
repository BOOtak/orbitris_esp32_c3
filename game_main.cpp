#include "game_main.h"

#include "game_screen.h"
#include "screen.h"
#include "sharp_display.h"

Screen* current_screen = nullptr;

void change_screen(Screen* from, Screen* to) {
  from->close();
  to->init();
  current_screen = to;
}

void update_screen() {
  Screen* new_screen = current_screen->update();
  if (new_screen != current_screen) {
    change_screen(current_screen, new_screen);
  }
}

void init_game() {
  screens::game_screen = new GameScreen();
  current_screen = screens::game_screen;
}

void update_draw_frame() {
  update_screen();
  current_screen->draw();
  lcd_update();
}
