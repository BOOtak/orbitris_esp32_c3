#pragma once

#include <cstdint>

#include "game_utils.h"

struct DrawMask {
  uint8_t mask[8];
};

constexpr DrawMask operator~(const DrawMask& m) {
  return { (uint8_t)~m.mask[0],
           (uint8_t)~m.mask[1],
           (uint8_t)~m.mask[2],
           (uint8_t)~m.mask[3],
           (uint8_t)~m.mask[4],
           (uint8_t)~m.mask[5],
           (uint8_t)~m.mask[6],
           (uint8_t)~m.mask[7] };
}

void begin_scale(float scale);

void end_scale();

void begin_screen_scale(float scale);

void end_screen_scale();

void begin_mask(DrawMask draw_mask);

void end_mask();

void draw_pixel(int x, int y, int color);

void fill_scrfeen_buffer(int color);

void draw_rectangle(const Rectangle& rect, int color);

void draw_rectangle_checkerboard(int posX, int posY, int width, int height);

void draw_rectangle_lines(int posX, int posY, int width, int height, int color);

void draw_rectangle_lines_pattern(const Rectangle& rect, uint8_t pattern_size, uint8_t pattern);

/**
 * @brief Simple line drawing function using Bresenham's algorithm.
 */
void draw_line(int x0, int y0, int x1, int y1, int color);

int draw_line_pattern(int x0, int y0, int x1, int y1, int pattern_state, uint8_t pattern_size, uint8_t pattern);

/**
 * @brief Prints a C-string of text to the LCD with scaling.
 * Only supports manual newlines ('\n').
 *
 * @param x Initial X-coordinate on the display.
 * @param y Initial Y-coordinate on the display.
 * @param scale Integer scale factor.
 * @param text The ASCII C-string to display.
 * @param color Text color.
 */
void print_text(int x, int y, int scale, const char* text, int color);

Vector2 measure_text(const char* text, int scale);
