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

/**
 * @brief Structure to hold the key analytical orbital elements.
 */
struct OrbitalElements {
  float semi_latus_rectum;  // p
  float eccentricity;       // e
  float arg_periapsis;      // omega
};

float distance_acceleration(const PlanetState& state, float star_mass);

float angle_acceleratioin(const PlanetState& state);

float new_value(float current_value, float delta_time, float derivative);

Vector2 state_to_coords(const PlanetState& state, float scale, Vector2 center);

void update_planet_state(PlanetState& state, float dt, float star_mass);

/**
 * @brief Calculates the orbital elements (p, e, omega) from the current planet state.
 * @param state The current PlanetState.
 * @param star_mass The mass of the central star.
 * @return OrbitalElements The structure containing p, e, and omega.
 */
OrbitalElements calc_orbital_elements(const PlanetState& state, float star_mass);