#pragma once

#include "game_utils.h"

struct SpVector2 {
  float value;
  float speed;
};

struct PlanetState {
  SpVector2 distance;
  SpVector2 angle;
};

float distance_acceleration(PlanetState state, float star_mass);

float angle_acceleratioin(PlanetState state);

float new_value(float current_value, float delta_time, float derivative);

Vector2 state_to_coords(const PlanetState& state, float scale, Vector2 center);

void update_planet_state(PlanetState& state, float dt, float star_mass);
