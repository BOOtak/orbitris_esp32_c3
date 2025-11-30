#include "table_math.h"

#include <cstdlib>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr auto TABLE_STEPS = 90;
constexpr auto TABLE_SIZE = TABLE_STEPS + 1;
constexpr float MAX_ANGLE = M_PI / 2.0f;
constexpr float TWO_PI = 2.0f * M_PI;
constexpr float ANGLE_STEP_RADIANS = MAX_ANGLE / TABLE_STEPS;

static float sin_table[TABLE_SIZE]{};

/**
 * @brief Initialize the sine lookup table (0 to PI/2 radians).
 */
void init_trig_tables() {
  for (int i = 0; i < TABLE_SIZE; ++i) {
    float angle_rad = (float)i * ANGLE_STEP_RADIANS;
    sin_table[i] = sinf(angle_rad);
  }
}

/**
 * @brief Calculate sin of an angle in radians using the pre-calculated table.
 * @param angle_radians The input angle in radians.
 * @return The approximate sine value.
 */
float approx_sin(float angle_radians) {
  float normalized_angle = fmodf(angle_radians, TWO_PI);
  if (normalized_angle < 0.0f) {
    normalized_angle += TWO_PI;
  }

  float reference_angle;
  int sign = 1;

  if (normalized_angle <= MAX_ANGLE) {  // [0, PI/2]
    reference_angle = normalized_angle;
    sign = 1;
  } else if (normalized_angle <= M_PI) {  // (PI/2, PI]
    reference_angle = M_PI - normalized_angle;
    sign = 1;
  } else if (normalized_angle <= M_PI + MAX_ANGLE) {  // (PI, 3PI/2]
    reference_angle = normalized_angle - M_PI;
    sign = -1;
  } else {  // (3PI/2, 2PI)
    reference_angle = TWO_PI - normalized_angle;
    sign = -1;
  }

  if (reference_angle < 0.0f) {
    reference_angle = 0.0f;
  }

  if (reference_angle > MAX_ANGLE) {
    reference_angle = MAX_ANGLE;
  }

  int index = (int)(reference_angle / ANGLE_STEP_RADIANS);

  if (index >= TABLE_SIZE) {
    index = TABLE_SIZE - 1;
  }

  return sin_table[index] * sign;
}


/**
 * @brief Calculate the cosine of an angle in radians using the lookup table.
 * @param angle_radians The input angle in radians.
 * @return The approximate cosine value.
 */
float approx_cos(float angle_radians) {
  return approx_sin(MAX_ANGLE - angle_radians);
}
