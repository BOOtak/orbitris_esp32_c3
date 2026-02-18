#include "../orbitris_esp32/input.h"

#include "raylib.h"

void input_init() {
}

void input_update() {
}

static int get_raylib_key(int key) {
  switch (key) {
    case ESP_KEY_UP:
      return KeyboardKey::KEY_UP;
    case ESP_KEY_DOWN:
      return KeyboardKey::KEY_DOWN;
    case ESP_KEY_LEFT:
      return KeyboardKey::KEY_LEFT;
    case ESP_KEY_RIGHT:
      return KeyboardKey::KEY_RIGHT;
    case ESP_KEY_A:
      return KeyboardKey::KEY_Z;
    case ESP_KEY_B:
      return KeyboardKey::KEY_X;
    default:
      return 0;
  }
}

bool is_key_down(int key) {
  return IsKeyDown(get_raylib_key(key));
}

bool is_key_pressed(int key) {
  return IsKeyPressed(get_raylib_key(key));
}

bool is_key_released(int key) {
  return IsKeyReleased(get_raylib_key(key));
}
