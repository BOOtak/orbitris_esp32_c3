#include "../orbitris_esp32/input.h"

#include "raylib.h"

void input_init() {
}

void input_update() {
}

bool is_key_down(int key) {
  switch (key) {
    case ESP_KEY_UP:
      return IsKeyDown(KeyboardKey::KEY_UP);
    case ESP_KEY_DOWN:
      return IsKeyDown(KeyboardKey::KEY_DOWN);
    case ESP_KEY_LEFT:
      return IsKeyDown(KeyboardKey::KEY_LEFT);
    case ESP_KEY_RIGHT:
      return IsKeyDown(KeyboardKey::KEY_RIGHT);
    case ESP_KEY_A:
      return IsKeyDown(KeyboardKey::KEY_Z);
    case ESP_KEY_B:
      return IsKeyDown(KeyboardKey::KEY_X);
    default:
      return false;
  }
  return false;
}

bool is_key_pressed(int key) {
  switch (key) {
    case ESP_KEY_UP:
      return IsKeyPressed(KeyboardKey::KEY_UP);
    case ESP_KEY_DOWN:
      return IsKeyPressed(KeyboardKey::KEY_DOWN);
    case ESP_KEY_LEFT:
      return IsKeyPressed(KeyboardKey::KEY_LEFT);
    case ESP_KEY_RIGHT:
      return IsKeyPressed(KeyboardKey::KEY_RIGHT);
    case ESP_KEY_A:
      return IsKeyPressed(KeyboardKey::KEY_Z);
    case ESP_KEY_B:
      return IsKeyPressed(KeyboardKey::KEY_X);
    default:
      return false;
  }
  return false;
}
