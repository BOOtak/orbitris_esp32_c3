#include "orbital.h"

#include <cmath>

constexpr float gravity_const = 6.67408E-11;

float distance_acceleration(const PlanetState& state, float star_mass) {
  return state.distance.value * pow(state.angle.speed, 2) - (gravity_const * star_mass) / pow(state.distance.value, 2);
}

float angle_acceleratioin(const PlanetState& state) {
  return -2.0f * state.distance.speed * state.angle.speed / state.distance.value;
}

float new_value(float current_value, float delta_time, float derivative) {
  return current_value + delta_time * derivative;
}

Vector2 state_to_coords(const PlanetState& state, float scale, Vector2 center) {
  Vector2 result;
  float dist_to_scale = state.distance.value / scale;
  result.x = center.x + dist_to_scale * cos(state.angle.value);
  result.y = center.y + dist_to_scale * sin(state.angle.value);
  return result;
}

void update_planet_state(PlanetState& state, float dt, float star_mass) {
  // Calculate initial accelerations
  float a_r_old = distance_acceleration(state, star_mass);  // a_r(t)
  float a_theta_old = angle_acceleratioin(state);           // a_theta(t)

  // Update speeds (v(t + dt/2)) for a half-step
  float dt_half = dt * 0.5f;
  state.distance.speed += a_r_old * dt_half;   // v_r(t + dt/2)
  state.angle.speed += a_theta_old * dt_half;  // v_theta(t + dt/2)

  // Update positions (r(t + dt)) for a full step using the half-step speed
  // r(t + dt) = r(t) + v(t + dt/2) * dt
  state.distance.value += state.distance.speed * dt;  // r(t + dt)
  state.angle.value += state.angle.speed * dt;        // theta(t + dt)

  // Calculate accelerations at t + dt
  float a_r_new = distance_acceleration(state, star_mass);  // a_r(t + dt)
  float a_theta_new = angle_acceleratioin(state);           // a_theta(t + dt)

  // Update speeds (v(t + dt)) for the final half-step
  // v(t + dt) = v(t + dt/2) + a(t + dt) * dt/2
  state.distance.speed += a_r_new * dt_half;   // v_r(t + dt)
  state.angle.speed += a_theta_new * dt_half;  // v_theta(t + dt)
}
