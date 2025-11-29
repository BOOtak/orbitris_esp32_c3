#include "orbital.h"

#include <cmath>

#include "trace.h"

constexpr float GRAVITY_CONST = 6.67408E-11;

float distance_acceleration(const PlanetState& state, float star_mass) {
  return state.distance.value * pow(state.angle.speed, 2) - (GRAVITY_CONST * star_mass) / pow(state.distance.value, 2);
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

OrbitalElements calc_orbital_elements(const PlanetState& state, float star_mass) {
  const float r = state.distance.value;
  if (r <= 0.0f) {
    return {};
  }

  OrbitalElements elements{};

  const float v_r = state.distance.speed;
  const float v_theta = state.angle.speed;
  const float mu = GRAVITY_CONST * star_mass;

  // Specific Angular Momentum (h = r^2 * v_theta)
  const float h = r * r * v_theta;

  // Specific Orbital Energy (epsilon = 0.5 * (v_r^2 + r^2 * v_theta^2) - mu/r)
  const float kinetic_energy = 0.5f * (v_r * v_r + powf(r * v_theta, 2.0f));
  const float potential_energy = -mu / r;
  const float epsilon = kinetic_energy + potential_energy;

  // Calculate Orbital Elements
  // Semi-Latus Rectum (p = h^2 / mu)
  elements.semi_latus_rectum = (h * h) / mu;

  // Eccentricity (e = sqrt(1 + (2 * epsilon * h^2) / mu^2))
  float h_over_mu = h / mu;  // avoid float overflow as mu^2 > 10^38
  float e_squared_term = 2.0f * epsilon * h_over_mu * h_over_mu;
  float eccentricity_squared = 1.0f + e_squared_term;
  if (eccentricity_squared < 0.0f) {
    eccentricity_squared = 0.0f;
  }

  elements.eccentricity = sqrtf(eccentricity_squared);

  if (elements.eccentricity < 1e-6) {
    // If the orbit is circular (e ~ 0), orientation is meaningless.
    // Set omega to 0 or current theta, depending on your convention.
    elements.arg_periapsis = 0.0f;
  } else {
    const float cos_anomaly_numerator = (h * h) - (r * mu);
    const float cos_anomaly_denominator = r * mu * elements.eccentricity;

    const float sin_anomaly_numerator = h * v_r;
    const float sin_anomaly_denominator = mu * elements.eccentricity;

    const float cos_anomaly = cos_anomaly_numerator / cos_anomaly_denominator;
    const float sin_anomaly = sin_anomaly_numerator / sin_anomaly_denominator;

    elements.arg_periapsis = state.angle.value - atan2f(sin_anomaly, cos_anomaly);
  }

  return elements;
}