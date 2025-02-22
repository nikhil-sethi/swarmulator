#include "agent.h"

Agent::Agent()
{
  activated = false;
}

// float Agent::get_position(uint16_t dim, bool noise)
// {
//   if (noise) return state.pose.pos[dim] + rg.gaussian_float(0.0, NOISE_R);
//   else return state.pose.pos[dim];
// }

Eigen::Vector3f Agent::get_position(bool noise)
{
  if (noise) return state.pose.pos + rg.gaussian_float_vector3(0.0, NOISE_R);
  else return state.pose.pos;
}

float Agent::get_velocity(uint16_t dim, bool noise)
{
  if (noise) return state.vel[dim] + rg.gaussian_float(0.0, NOISE_R);
  else return state.vel[dim];
}

float Agent::get_accel(uint16_t dim, bool noise)
{
  if (noise) return state.acc[dim] + rg.gaussian_float(0.0, NOISE_R);
  else return state.acc[dim];
}

float Agent::get_orientation(bool noise)
{
  if (noise) return state.psi + rg.gaussian_float(0.0, NOISE_R);
  else return state.psi;
}

// float Agent::get_state(const uint16_t i)
// {
//   float noise = rg.gaussian_float(0.0, NOISE_R);
//   return state[i] + noise;
// }
