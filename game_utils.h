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

bool check_collision_recs(const Rectangle& rec1, const Rectangle& rec2);

// Get collision rectangle for two rectangles collision
Rectangle get_collision_rec(const Rectangle& rec1, const Rectangle& rec2);

// Get a random value between min and max (both included)
int get_random_value(int min, int max);
