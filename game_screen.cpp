#include "game_screen.h"
#include "sharp_display.h"

GameScreen::GameScreen()
  : Screen() {}

Screen* GameScreen::update() {
  frame_++;
  if (frame_ > LCD_WIDTH) {
    frame_ = 0;
  }
  return this;
}

void GameScreen::draw() {
  lcd_fill_buffer(1);
  lcd_draw_pixel(frame_, 0, 1);
  lcd_draw_pixel(frame_, 1, 0);
  lcd_draw_pixel(frame_, 2, 1);
  lcd_draw_pixel(frame_, 3, 0);
}
