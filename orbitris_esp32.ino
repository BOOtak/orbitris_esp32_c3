#include <Arduino.h>
#include <stdlib.h>  // For abs()

#include "game_main.h"
#include "input.h"
#include "sharp_display.h"

#define TARGET_FPS 60
#define FRAME_BUDGET_US (1000000 / TARGET_FPS)

// FPS counter
static float framesPerSecond = 0.0f;
static uint64_t frameCount = 0;
static uint64_t lastMillis = 0;


/**
 * @brief Simple line drawing function using Bresenham's algorithm.
 */
void draw_line(int x0, int y0, int x1, int y1, int color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2, e2;

  for (;;) {
    lcd_draw_pixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}

void setup() {
  Serial.begin(115200);

  lcd_init();

  input_init();

  lcd_clear();
  lcd_fill_buffer(1);  // Fill buffer with white (1)
  lcd_update();

  init_game();
}

float fps(int seconds) {
  frameCount++;
  unsigned long currentMillis = millis();
  if ((currentMillis - lastMillis) > (seconds * 1000)) {
    framesPerSecond = ((float)frameCount) / seconds;
    frameCount = 0;
    lastMillis = currentMillis;
    Serial.printf("%04.3f\n", framesPerSecond);
  }
  return framesPerSecond;
}

void loop() {
  uint32_t ts = micros();
  update_draw_frame();
  fps(1);
  uint32_t dt = micros() - ts;
  if (dt < FRAME_BUDGET_US) {
    delayMicroseconds(FRAME_BUDGET_US - dt);
  }
}
