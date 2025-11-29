// Sharp Memory LCD LS027B7DH01 (400x240) Driver for ESP32 Arduino

#pragma once

void lcd_init();

/**
 * @brief Sends framebuffer data to the display
 */
void lcd_update();

/**
 * @brief Sends an All Clear command to the display.
 */
void lcd_clear();

/**
 * @brief Updates a single pixel in the local framebuffer.
 * @param x X coordinate (0 to LCD_WIDTH-1).
 * @param y Y coordinate (0 to LCD_HEIGHT-1).
 * @param color Color (1 for black/set, 0 for white/clear).
 */
void lcd_draw_pixel(int x, int y, int color);

/**
 * @brief Fills the local framebuffer with a single color (0 for white, 1 for black).
 * @param color The color to fill (0 or 1).
 */
void lcd_fill_buffer(int color);
