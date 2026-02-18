#pragma once

#define ARR_SIZE(X) (sizeof(X) / sizeof(X[0]))

struct Vector2 {
  float x;
  float y;
};

inline Vector2 operator+(const Vector2& v1, const Vector2& v2) {
  return { v1.x + v2.x, v1.y + v2.y };
}

inline Vector2 operator*(const Vector2& v, float scale) {
  return { v.x * scale, v.y * scale };
}

inline Vector2 operator-(const Vector2& v1, const Vector2& v2) {
  return { v1.x - v2.x, v1.y - v2.y };
}

inline void operator+=(Vector2& v1, const Vector2& v2) {
  v1.x += v2.x;
  v1.y += v2.y;
}

struct Rectangle {
  float x;       // Rectangle top-left corner position x
  float y;       // Rectangle top-left corner position y
  float width;   // Rectangle width
  float height;  // Rectangle height
};

float remap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd);

float my_lerp(float start, float end, float amount);

bool check_collision_recs(const Rectangle& rec1, const Rectangle& rec2);

// Get collision rectangle for two rectangles collision
Rectangle get_collision_rec(const Rectangle& rec1, const Rectangle& rec2);

// Get a random value between min and max (both included)
int get_random_value(int min, int max);

Vector2 vector2_lerp(const Vector2& v1, const Vector2& v2, float amount);

float vector2_square_dist(const Vector2& v1, const Vector2& v2);

float vector2_dist(const Vector2& v1, const Vector2& v2);

float ease_out_cubic(float x);

float ease_out_quad(float x);

float ease_out_circ(float x);
