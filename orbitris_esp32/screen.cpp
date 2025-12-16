#include "screen.h"

Screen::Screen() {}

void Screen::init() {}

void Screen::draw() const {}

void Screen::close() {}

namespace screens {
Screen* game_screen = nullptr;
Screen* game_over_screen = nullptr;
}
