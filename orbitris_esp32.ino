#include <Arduino.h>
#include <stdlib.h>  // For abs()

#include "sharp_display.h"

#define PIN_BTN_UP 3
#define PIN_BTN_DOWN 0
#define PIN_BTN_LEFT 1
#define PIN_BTN_RIGHT 2

#define PIN_BTN_A 10
#define PIN_BTN_B 20

static int draw_speed = 50;
static float center_x = 0.0f;
static float center_y = 0.0f;

// FPS counter
static float framesPerSecond = 0.0f;
static uint64_t frameCount = 0;
static uint64_t lastMillis = 0;

#define EPSILON 0.00001

#define W3D 4.0f
#define H3D 2.4f

struct Vec3 {
  float x;
  float y;
  float z;
};

/**
 * @brief Initializes the GPIOs for CS, DISP, and VCOM using Arduino functions.
 */
void gpio_init() {
  lcd_init();

  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);

  Serial.println("GPIO and control pins initialized.");
}

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

void draw_line_3d(Vec3& v1, Vec3& v2) {
  // Convert from 3D space to 2D space
  // From:
  //         Z
  //         ^
  //         |
  //         |
  //         |
  //         |
  //         +-----------> X
  //        /
  //       /
  //      /
  //     Y
  //
  // (0;0) is at a center of the screen

  // Serial.printf("%.3f %.3f %.3f %.3f %.3f %.3f\n", v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);

  // Show only visible part of the line
  if (v1.y < EPSILON && v2.y < EPSILON) {
    return;
  }

  // Adjust visible line length
  if (v1.y < EPSILON) {
    float frac = abs(v2.y) / abs(v1.y - v2.y);
    v1.x = frac * (v1.x - v2.x) + v2.x;
    v1.y = EPSILON;
    v1.z = frac * (v1.z - v2.z) + v2.z;
  } else if (v2.y < EPSILON) {
    float frac = abs(v1.y) / abs(v1.y - v2.y);
    v2.x = frac * (v2.x - v1.x) + v1.x;
    v2.y = EPSILON;
    v2.z = frac * (v2.z - v1.z) + v1.z;
  }

  // Project to 2d space
  float x1 = v1.x / v1.y;
  float y1 = v1.z / v1.y;
  float x2 = v2.x / v2.y;
  float y2 = v2.z / v2.y;

  // Serial.printf(" -> %.3f %.3f %.3f %.3f\n", x1, y1, x2, y2);

  int x1norm = ((x1 + W3D / 2) / W3D) * LCD_WIDTH;
  int y1norm = ((y1 + H3D / 2) / H3D) * LCD_HEIGHT;
  int x2norm = ((x2 + W3D / 2) / W3D) * LCD_WIDTH;
  int y2norm = ((y2 + H3D / 2) / H3D) * LCD_HEIGHT;

  // Serial.printf(" -> %3d %3d %3d %3d\n", x1norm, y1norm, x2norm, y2norm);

  draw_line(x1norm, y1norm, x2norm, y2norm, 0);
}

void calcCellVec(int x, int y, int max_x, int max_y, float phi, Vec3& out) {
  float xnorm = ((float)x / max_x) - 0.5f + center_x;
  float ynorm = ((float)y / max_y) - 0.5f + center_y;
  out.x = (xnorm * W3D);
  out.y = (ynorm * H3D) + H3D;
  float dist = sqrt(xnorm * xnorm + ynorm * ynorm);
  out.z = cos((dist + phi) * PI * 3) * (1 - dist) + H3D / 2;
}

void draw_cool_pattern(int dt) {
  float phi = (float)dt / (PI / 2 * draw_speed);
  lcd_fill_buffer(1);
  int cells_x = 16, cells_y = 16;
  Vec3 v1{};
  Vec3 v2{};
  for (int x = 0; x < cells_x; x++) {
    calcCellVec(x, 0, cells_x, cells_y, phi, v1);
    for (int y = 0; y < cells_y; y++) {
      if (x < cells_x - 1) {
        calcCellVec(x + 1, y, cells_x, cells_y, phi, v2);
        draw_line_3d(v1, v2);
      }
      if (y < cells_y - 1) {
        calcCellVec(x, y + 1, cells_x, cells_y, phi, v2);
        draw_line_3d(v1, v2);
        v1 = v2;
      }
    }
  }
}

void scan_keys() {
  float d_center = 0.01f;
  if (!digitalRead(PIN_BTN_UP)) {
    Serial.println("UP!");
    center_y += d_center;
  }
  if (!digitalRead(PIN_BTN_DOWN)) {
    Serial.println("DOWN!");
    center_y -= d_center;
  }
  if (!digitalRead(PIN_BTN_LEFT)) {
    Serial.println("LEFT!");
    center_x -= d_center;
  }
  if (!digitalRead(PIN_BTN_RIGHT)) {
    Serial.println("RIGHT!");
    center_x += d_center;
  }
  if (!digitalRead(PIN_BTN_A)) {
    Serial.println("A!");
    draw_speed += 1;
  }
  if (!digitalRead(PIN_BTN_B)) {
    Serial.println("B!");
    draw_speed -= 1;
  }
}

void setup() {
  Serial.begin(115200);
  // Give time for serial monitor to connect
  delay(1000);

  // 1. Initialize GPIOs (VCOM, DISP, CS)
  gpio_init();

  lcd_fill_buffer(1);  // Fill buffer with white (0)
  lcd_clear();

  // 4. Draw test patterns on the local buffer
  Serial.println("Drawing test patterns...");
  int w = LCD_WIDTH - 1;
  int h = LCD_HEIGHT - 1;
  int c = 0;  // Black color (1)

  // Outer border
  draw_line(0, 0, w, 0, c);
  draw_line(w, 0, w, h, c);
  draw_line(w, h, 0, h, c);
  draw_line(0, h, 0, 0, c);

  // Diagonals
  draw_line(0, 0, w, h, c);
  draw_line(w, 0, 0, h, c);
  // 5. Update the display with the new content
  lcd_update();
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
  // VCOM Polarity must be periodically toggled (e.g., every 2 seconds)
  // to prevent DC bias degradation on the display's liquid crystal.
  static int cycle_count = 0;
  draw_cool_pattern(cycle_count);
  scan_keys();
  lcd_update();
  fps(1);
  cycle_count++;
}
