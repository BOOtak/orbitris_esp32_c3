#pragma once

#include <cstdint>
#include "game_utils.h"

/**
 * @file draw.h
 * @brief Drawing functions for LCD display with scaling and masking support
 *
 * This module provides drawing primitives with support for:
 * - Coordinate scaling (both draw and screen scaling)
 * - Pixel masking for pattern-based drawing
 * - Various shape drawing (pixels, rectangles, lines)
 * - Text rendering with bitmap fonts
 */

/**
 * @struct DrawMask
 * @brief 8x8 pixel mask for pattern-based drawing
 *
 * Each byte in the mask array represents one row of 8 pixels.
 * Bit 7 (0x80) corresponds to the leftmost pixel in the row.
 * A bit value of 1 allows drawing, 0 prevents drawing.
 */
struct DrawMask {
  uint8_t mask[8];  ///< 8 rows of 8 pixels each
};

/**
 * @brief Bitwise NOT operator for DrawMask
 * @param m The mask to invert
 * @return A new DrawMask with all bits inverted
 *
 * Creates a mask where previously allowed pixels become blocked
 * and vice versa. Useful for creating inverse patterns.
 */
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

/**
 * @brief Begins a scaling block for drawing operations
 * @param scale Scale factor (1.0 = no scaling)
 *
 * All subsequent drawing operations will have their coordinates
 * scaled relative to the screen center (CENTER_X, CENTER_Y).
 * Must be paired with end_scale().
 */
void begin_scale(float scale);

/**
 * @brief Ends the current scaling block
 * Resets draw scaling to 1.0.
 */
void end_scale();

/**
 * @brief Begins screen-level scaling
 * @param scale Screen scale factor (1.0 = no scaling)
 *
 * Screen scaling is multiplied with draw scaling to produce
 * the final scale factor. Useful for global zoom effects.
 */
void begin_screen_scale(float scale);

/**
 * @brief Ends screen-level scaling
 * Resets screen scaling to 1.0.
 */
void end_screen_scale();

/**
 * @brief Begins a masking block for pattern-based drawing
 * @param draw_mask The 8x8 mask pattern to apply
 *
 * When masking is active, pixels are only drawn where the
 * corresponding bit in the mask is set. The mask repeats every
 * 8 pixels in both X and Y directions.
 */
void begin_mask(DrawMask draw_mask);

/**
 * @brief Ends the current masking block
 * Disables pixel masking for subsequent drawing operations.
 */
void end_mask();

/**
 * @brief Draws a single pixel
 * @param x X-coordinate
 * @param y Y-coordinate
 * @param color Pixel color value
 *
 * Applies current scaling and masking settings.
 */
void draw_pixel(int x, int y, int color);

/**
 * @brief Fills the entire screen buffer with a color
 * @param color Fill color
 *
 * If masking is active, fills each line with the mask pattern.
 */
void fill_screen_buffer(int color);

/**
 * @brief Draws a filled rectangle
 * @param rect Rectangle structure containing position and size
 * @param color Fill color
 */
void draw_rectangle(const Rectangle& rect, int color);

/**
 * @brief Draws a filled rectangle
 * @param posX Left edge X-coordinate
 * @param posY Top edge Y-coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color Fill color
 */
void draw_rectangle(int posX, int posY, int width, int height, int color);

/**
 * @brief Draws a checkerboard pattern rectangle
 * @param posX Left edge X-coordinate
 * @param posY Top edge Y-coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 *
 * Creates a checkerboard pattern by drawing pixels at positions
 * where (x + y) is odd.
 */
void draw_rectangle_checkerboard(int posX, int posY, int width, int height);

/**
 * @brief Draws only the outline of a rectangle
 * @param posX Left edge X-coordinate
 * @param posY Top edge Y-coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color Line color
 */
void draw_rectangle_lines(int posX, int posY, int width, int height, int color);

/**
 * @brief Draws a rectangular outline with a pattern
 * @param rect Rectangle structure containing position and size
 * @param pattern_size Number of bits in pattern (1-8)
 * @param pattern Bit pattern for the line (MSB first)
 *
 * The pattern repeats along the rectangle perimeter. For each pixel,
 * if the corresponding pattern bit is 1, the pixel is drawn.
 */
void draw_rectangle_lines_pattern(const Rectangle& rect, uint8_t pattern_size, uint8_t pattern);

/**
 * @brief Draws a line using Bresenham's algorithm
 * @param x0 Start point X-coordinate
 * @param y0 Start point Y-coordinate
 * @param x1 End point X-coordinate
 * @param y1 End point Y-coordinate
 * @param color Line color
 */
void draw_line(int x0, int y0, int x1, int y1, int color);

/**
 * @brief Draws a horizontal line
 * @param x Start X-coordinate
 * @param y Y-coordinate
 * @param length Line length in pixels
 * @param color Line color
 */
void draw_hline(int x, int y, int length, int color);

/**
 * @brief Draws a patterned line
 * @param x0 Start point X-coordinate
 * @param y0 Start point Y-coordinate
 * @param x1 End point X-coordinate
 * @param y1 End point Y-coordinate
 * @param pattern_state Initial pattern position
 * @param pattern_size Number of bits in pattern (1-8)
 * @param pattern Bit pattern for the line (MSB first)
 * @return Updated pattern state after drawing
 *
 * Draws a line where each pixel's visibility is controlled by
 * the pattern. Useful for dashed or dotted lines.
 */
int draw_line_pattern(int x0, int y0, int x1, int y1, int pattern_state, uint8_t pattern_size, uint8_t pattern);

/**
 * @brief Prints text using the standard bitmap font
 * @param x Initial X-coordinate
 * @param y Initial Y-coordinate
 * @param scale Integer scale factor (must be >= 1)
 * @param text ASCII C-string to display
 * @param color Text color
 *
 * Supports newline characters ('\n') for multi-line text.
 * Text automatically stops if it would exceed screen boundaries.
 */
void print_text(int x, int y, int scale, const char* text, int color);

/**
 * @brief Prints text with extended font option
 * @param x Initial X-coordinate
 * @param y Initial Y-coordinate
 * @param scale Integer scale factor (must be >= 1)
 * @param text ASCII C-string to display
 * @param color Text color
 * @param extended_font If true, uses extended character set from extended_charmap
 *
 * Extended font includes additional symbols and special characters.
 */
void print_text(int x, int y, int scale, const char* text, int color, bool extended_font);

/**
 * @brief Calculates the dimensions of text without drawing it
 * @param text The text to measure
 * @param scale Scale factor to apply
 * @return Vector2 containing width and height of the text
 *
 * Accounts for newlines when calculating total dimensions.
 * Width is based on the longest line.
 */
Vector2 measure_text(const char* text, int scale);
