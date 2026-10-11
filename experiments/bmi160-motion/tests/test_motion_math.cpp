#include <cassert>
#include <cmath>

#include "../motion_math.h"

namespace {

bool near(double actual, double expected, double tolerance = 1e-9) {
  return std::fabs(actual - expected) <= tolerance;
}

}  // namespace

int main() {
  MotionIntegrator constantAcceleration;
  assert(constantAcceleration.step({2.0, 0.0, 0.0}, 1.0));
  assert(constantAcceleration.step({2.0, 0.0, 0.0}, 1.0));
  assert(near(constantAcceleration.velocity().x, 4.0));
  assert(near(constantAcceleration.displacement().x, 4.0));

  MotionIntegrator accelerateCoastDecelerate;
  assert(accelerateCoastDecelerate.step({1.0, 0.0, 0.0}, 1.0));
  assert(accelerateCoastDecelerate.step({1.0, 0.0, 0.0}, 1.0));
  assert(accelerateCoastDecelerate.step({0.0, 0.0, 0.0}, 1.0));
  assert(accelerateCoastDecelerate.step({0.0, 0.0, 0.0}, 1.0));
  assert(near(accelerateCoastDecelerate.velocity().x, 2.5));
  assert(accelerateCoastDecelerate.step({-1.0, 0.0, 0.0}, 1.0));
  assert(accelerateCoastDecelerate.step({-1.0, 0.0, 0.0}, 1.0));
  assert(accelerateCoastDecelerate.step({-1.0, 0.0, 0.0}, 1.0));
  assert(near(accelerateCoastDecelerate.velocity().x, 0.0));

  MotionIntegrator constantVelocity;
  constantVelocity.reset({1.0, 0.0, 0.0});
  assert(constantVelocity.step({0.0, 0.0, 0.0}, 1.0));
  assert(near(constantVelocity.velocity().x, 1.0));
  assert(near(constantVelocity.displacement().x, 1.0));

  MotionIntegrator biasedAcceleration;
  constexpr double bias = 0.1;
  constexpr int durationSeconds = 10;
  for (int second = 0; second < durationSeconds; ++second) {
    assert(biasedAcceleration.step({bias, 0.0, 0.0}, 1.0));
  }
  assert(near(biasedAcceleration.displacement().x,
              0.5 * bias * durationSeconds * durationSeconds));

  MotionIntegrator signedAxes;
  assert(signedAxes.step({0.0, -2.0, 1.0}, 1.0));
  assert(signedAxes.step({0.0, -2.0, 1.0}, 1.0));
  assert(near(signedAxes.velocity().y, -4.0));
  assert(near(signedAxes.displacement().z, 2.0));

  assert(!signedAxes.step({0.0, 0.0, 0.0}, 0.0));
  assert(!signedAxes.step({NAN, 0.0, 0.0}, 1.0));
  return 0;
}
