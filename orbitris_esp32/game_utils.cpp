#include "game_utils.h"

#include <cmath>
#include <cstdlib>

float remap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd) {
  return (value - inputStart) / (inputEnd - inputStart) * (outputEnd - outputStart) + outputStart;
}

float my_lerp(float start, float end, float amount) {
  return start + amount * (end - start);
}

bool check_collision_recs(const Rectangle& rec1, const Rectangle& rec2) {
  bool collision = false;

  if ((rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x)
      && (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y)) {
    collision = true;
  }

  return collision;
}

Rectangle get_collision_rec(const Rectangle& rec1, const Rectangle& rec2) {
  Rectangle rec = { 0, 0, 0, 0 };

  if (check_collision_recs(rec1, rec2)) {
    float dxx = fabsf(rec1.x - rec2.x);
    float dyy = fabsf(rec1.y - rec2.y);

    if (rec1.x <= rec2.x) {
      if (rec1.y <= rec2.y) {
        rec.x = rec2.x;
        rec.y = rec2.y;
        rec.width = rec1.width - dxx;
        rec.height = rec1.height - dyy;
      } else {
        rec.x = rec2.x;
        rec.y = rec1.y;
        rec.width = rec1.width - dxx;
        rec.height = rec2.height - dyy;
      }
    } else {
      if (rec1.y <= rec2.y) {
        rec.x = rec1.x;
        rec.y = rec2.y;
        rec.width = rec2.width - dxx;
        rec.height = rec1.height - dyy;
      } else {
        rec.x = rec1.x;
        rec.y = rec1.y;
        rec.width = rec2.width - dxx;
        rec.height = rec2.height - dyy;
      }
    }

    if (rec1.width > rec2.width) {
      if (rec.width >= rec2.width) rec.width = rec2.width;
    } else {
      if (rec.width >= rec1.width) rec.width = rec1.width;
    }

    if (rec1.height > rec2.height) {
      if (rec.height >= rec2.height) rec.height = rec2.height;
    } else {
      if (rec.height >= rec1.height) rec.height = rec1.height;
    }
  }

  return rec;
}

int get_random_value(int min, int max) {
  if (min > max) {
    int tmp = max;
    max = min;
    min = tmp;
  }

  return (rand() % (abs(max - min) + 1) + min);
}

Vector2 vector2_lerp(const Vector2& v1, const Vector2& v2, float amount) {
  Vector2 result = {};

  result.x = v1.x + amount * (v2.x - v1.x);
  result.y = v1.y + amount * (v2.y - v1.y);

  return result;
}

float vector2_square_dist(const Vector2& v1, const Vector2& v2) {
  return (v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y);
}

float vector2_dist(const Vector2& v1, const Vector2& v2) {
  return sqrt(vector2_square_dist(v1, v2));
}
