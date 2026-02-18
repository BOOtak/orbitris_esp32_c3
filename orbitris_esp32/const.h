#pragma once

constexpr auto LCD_WIDTH = 400;
constexpr auto LCD_HEIGHT = 240;
constexpr int CENTER_X = LCD_WIDTH / 2;
constexpr int CENTER_Y = LCD_HEIGHT / 2;

constexpr auto TARGET_FPS = 60;
constexpr auto FRAME_BUDGET_US = 1000000 / TARGET_FPS;

constexpr int LCD_BLACK = 0;
constexpr int LCD_WHITE = 1;
