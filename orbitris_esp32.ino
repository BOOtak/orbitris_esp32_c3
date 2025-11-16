#include <Arduino.h>

#include "game_main.h"
#include "input.h"
#include "sharp_display.h"

#define TARGET_FPS 60
#define FRAME_BUDGET_US (1000000 / TARGET_FPS)

// FPS counter
static float framesPerSecond = 0.0f;
static uint64_t frameCount = 0;
static uint64_t lastMillis = 0;


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
  input_update();
  update_draw_frame();
  fps(1);
  uint32_t dt = micros() - ts;
  if (dt < FRAME_BUDGET_US) {
    delayMicroseconds(FRAME_BUDGET_US - dt);
  }
}
