#pragma once

/**
 * @brief Initialize the sine lookup table (0 to PI/2 radians).
 */
void init_trig_tables();

/**
 * @brief Calculate sin of an angle in radians using the pre-calculated table.
 * @param angle_radians The input angle in radians.
 * @return The approximate sine value.
 */
float approx_sin(float angle_radians);

/**
 * @brief Calculate the cosine of an angle in radians using the lookup table.
 * @param angle_radians The input angle in radians.
 * @return The approximate cosine value.
 */
float approx_cos(float angle_radians);
