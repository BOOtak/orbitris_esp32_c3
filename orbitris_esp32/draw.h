#pragma once

#include <cstdint>

void fill_scrfeen_buffer(int color);

void draw_rectangle_lines(int posX, int posY, int width, int height, int color);

/**
 * @brief Simple line drawing function using Bresenham's algorithm.
 */
void draw_line(int x0, int y0, int x1, int y1, int color);

int draw_line_pattern(int x0, int y0, int x1, int y1, int pattern_state, uint8_t pattern_size, uint8_t pattern);
