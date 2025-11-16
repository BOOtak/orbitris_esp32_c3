#pragma once

#define KEY_UP 3
#define KEY_DOWN 0
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_A 10
#define KEY_B 20

void input_init();

void input_update();

bool is_key_down(int key);

bool is_key_pressed(int key);
