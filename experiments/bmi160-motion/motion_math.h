#pragma once

#include <cmath>

struct Vector3 {
  double x;
  double y;
  double z;
};

inline Vector3 operator+(const Vector3& left, const Vector3& right) {
  return {left.x + right.x, left.y + right.y, left.z + right.z};
}

inline Vector3 operator*(const Vector3& value, double scale) {
  return {value.x * scale, value.y * scale, value.z * scale};
}

class MotionIntegrator {
 public:
  MotionIntegrator() { reset(); }

  void reset(const Vector3& initial_velocity = {0.0, 0.0, 0.0}) {
    velocity_ = initial_velocity;
    displacement_ = {0.0, 0.0, 0.0};
    previous_acceleration_ = {0.0, 0.0, 0.0};
    has_previous_acceleration_ = false;
  }

  bool step(const Vector3& acceleration, double dt_seconds) {
    if (!std::isfinite(dt_seconds) || dt_seconds <= 0.0 ||
        !isFinite(acceleration)) {
      return false;
    }

    if (!has_previous_acceleration_) {
      previous_acceleration_ = acceleration;
      has_previous_acceleration_ = true;
    }

    const Vector3 average_acceleration =
        (previous_acceleration_ + acceleration) * 0.5;
    const Vector3 next_velocity =
        velocity_ + average_acceleration * dt_seconds;
    displacement_ =
        displacement_ + (velocity_ + next_velocity) * (0.5 * dt_seconds);
    velocity_ = next_velocity;
    previous_acceleration_ = acceleration;
    return true;
  }

  void holdStationary(const Vector3& acceleration) {
    velocity_ = {0.0, 0.0, 0.0};
    previous_acceleration_ = acceleration;
    has_previous_acceleration_ = true;
  }

  const Vector3& velocity() const { return velocity_; }
  const Vector3& displacement() const { return displacement_; }

 private:
  static bool isFinite(const Vector3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
  }

  Vector3 velocity_;
  Vector3 displacement_;
  Vector3 previous_acceleration_;
  bool has_previous_acceleration_;
};
