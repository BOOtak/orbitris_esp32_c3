#pragma once

#define ESP_KEY_UP 3
#define ESP_KEY_DOWN 0
#define ESP_KEY_LEFT 1
#define ESP_KEY_RIGHT 2
#define ESP_KEY_A 10
#define ESP_KEY_B 20

void input_init();

void input_update();

bool is_key_down(int key);

bool is_key_pressed(int key);
