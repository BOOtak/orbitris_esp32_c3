#pragma once

#define ARR_SIZE(X) (sizeof(X) / sizeof(X[0]))

struct Vector2 {
  float x;
  float y;
};

struct Rectangle {
  float x;       // Rectangle top-left corner position x
  float y;       // Rectangle top-left corner position y
  float width;   // Rectangle width
  float height;  // Rectangle height
};

float remap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd);

float lerp(float start, float end, float amount);

bool check_collision_recs(const Rectangle& rec1, const Rectangle& rec2);

// Get collision rectangle for two rectangles collision
Rectangle get_collision_rec(const Rectangle& rec1, const Rectangle& rec2);

// Get a random value between min and max (both included)
int get_random_value(int min, int max);

Vector2 vector2_lerp(const Vector2& v1, const Vector2& v2, float amount);

Vector2 vector2_add(const Vector2& v1, const Vector2& v2);

Vector2 vector2_scale(const Vector2& v, float scale);

float vector2_square_dist(const Vector2& v1, const Vector2& v2);

float vector2_dist(const Vector2& v1, const Vector2& v2);
