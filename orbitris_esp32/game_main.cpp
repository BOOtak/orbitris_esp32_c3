#include "game_main.h"

#include "game_screen.h"
#include "game_over_screen.h"
#include "game_utils.h"
#include "screen.h"
#include "stats.h"
#include "table_math.h"
#include "transition.h"

Screen* current_screen = nullptr;
Screen* new_screen = nullptr;

Stats stats{};

static bool in_transition = false;

void update_screen() {
  new_screen = current_screen->update();
  if (new_screen != current_screen) {
    in_transition = true;
    start_transition(current_screen, new_screen, TransitionKind::ZOOM_IN);
  }
}

void draw_screen() {
  current_screen->draw();
}

void init_game() {
  init_trig_tables();
  screens::game_screen = new GameScreen(stats);
  screens::game_over_screen = new GameOverScreen(stats);
  current_screen = screens::game_screen;
  current_screen->init();
  in_transition = false;
}

void update_draw_frame() {
  if (in_transition) {
    in_transition = !update_transition();
    if (!in_transition) {
      current_screen = new_screen;
    }
  } else {
    update_screen();
  }

  if (in_transition) {
    draw_transition();
  } else {
    draw_screen();
  }
}
