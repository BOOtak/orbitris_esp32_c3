#include "input.h"

#include <Arduino.h>

#define IDX_UP 0
#define IDX_DOWN 1
#define IDX_LEFT 2
#define IDX_RIGHT 3
#define IDX_A 4
#define IDX_B 5
#define IDX_MAX IDX_B

static bool btn_states_prev[IDX_MAX]{};
static bool btn_states[IDX_MAX]{};

void input_init() {
  pinMode(ESP_KEY_UP, INPUT_PULLUP);
  pinMode(ESP_KEY_DOWN, INPUT_PULLUP);
  pinMode(ESP_KEY_LEFT, INPUT_PULLUP);
  pinMode(ESP_KEY_RIGHT, INPUT_PULLUP);
  pinMode(ESP_KEY_A, INPUT_PULLUP);
  pinMode(ESP_KEY_B, INPUT_PULLUP);
}

void input_update() {
  memcpy(btn_states_prev, btn_states, sizeof(btn_states));

  btn_states[IDX_UP] = !digitalRead(ESP_KEY_UP);
  btn_states[IDX_DOWN] = !digitalRead(ESP_KEY_DOWN);
  btn_states[IDX_LEFT] = !digitalRead(ESP_KEY_LEFT);
  btn_states[IDX_RIGHT] = !digitalRead(ESP_KEY_RIGHT);
  btn_states[IDX_A] = !digitalRead(ESP_KEY_A);
  btn_states[IDX_B] = !digitalRead(ESP_KEY_B);
}

bool is_key_down(int key) {
  return !digitalRead(key);
}

bool is_key_pressed(int key) {
  int idx = 0;
  switch (key) {
    case ESP_KEY_UP:
      idx = IDX_UP;
      break;
    case ESP_KEY_DOWN:
      idx = IDX_DOWN;
      break;
    case ESP_KEY_LEFT:
      idx = IDX_LEFT;
      break;
    case ESP_KEY_RIGHT:
      idx = IDX_RIGHT;
      break;
    case ESP_KEY_A:
      idx = IDX_A;
      break;
    case ESP_KEY_B:
      idx = IDX_B;
      break;
  }

  return btn_states[idx] && !btn_states_prev[idx];
}
