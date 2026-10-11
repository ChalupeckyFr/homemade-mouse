#include <Wire.h>

#include <math.h>
#include <stdio.h>

#include "motion_math.h"

namespace {

constexpr uint8_t kSdaPin = 20;
constexpr uint8_t kSclPin = 10;
constexpr uint8_t kCandidateAddresses[] = {0x68, 0x69};
constexpr uint8_t kChipIdRegister = 0x00;
constexpr uint8_t kExpectedChipId = 0xD1;
constexpr uint8_t kStatusRegister = 0x1B;
constexpr uint8_t kGyroDataRegister = 0x0C;
constexpr uint8_t kAccelConfigRegister = 0x40;
constexpr uint8_t kAccelRangeRegister = 0x41;
constexpr uint8_t kGyroConfigRegister = 0x42;
constexpr uint8_t kGyroRangeRegister = 0x43;
constexpr uint8_t kPmuStatusRegister = 0x03;
constexpr uint8_t kCommandRegister = 0x7E;
constexpr uint8_t kDataReadyMask = 0xC0;
constexpr uint32_t kOutputDataRateHz = 100;
constexpr double kNominalPeriodSeconds = 1.0 / kOutputDataRateHz;
constexpr double kGravity = 9.80665;
constexpr double kAccelCountsPerG = 16384.0;
constexpr double kGyroCountsPerDegreePerSecond = 131.0;
constexpr uint32_t kCalibrationDurationMs = 2000;
constexpr uint32_t kLongCalibrationDurationMs = 10000;
constexpr uint32_t kMinimumCalibrationSamples = 150;
constexpr uint32_t kStationaryHoldMs = 500;
constexpr double kMaxAccelStdDev = 0.20;
constexpr double kMaxGyroStdDev = 0.035;
constexpr double kMaxGyroMean = 0.08;
constexpr double kMinGravityMagnitude = 0.75 * kGravity;
constexpr double kMaxGravityMagnitude = 1.25 * kGravity;
constexpr double kStationaryAccelThreshold = 0.18;
constexpr double kStationaryGyroThreshold = 0.04;
constexpr size_t kCsvBufferSize = 512;

enum class RunState { kCalibrating, kRunning, kFatal };

struct RawSample {
  int16_t gyro[3];
  int16_t accel[3];
};

struct CalibrationStats {
  double sum[6] = {};
  double sumSquares[6] = {};
  uint32_t count = 0;

  void clear() {
    for (size_t axis = 0; axis < 6; ++axis) {
      sum[axis] = 0.0;
      sumSquares[axis] = 0.0;
    }
    count = 0;
  }

  void add(const Vector3& accel, const Vector3& gyro) {
    const double values[6] = {
        accel.x, accel.y, accel.z, gyro.x, gyro.y, gyro.z};
    for (size_t axis = 0; axis < 6; ++axis) {
      sum[axis] += values[axis];
      sumSquares[axis] += values[axis] * values[axis];
    }
    ++count;
  }

  double mean(size_t axis) const {
    return count == 0 ? 0.0 : sum[axis] / count;
  }

  double standardDeviation(size_t axis) const {
    if (count < 2) return INFINITY;
    const double average = mean(axis);
    const double variance = sumSquares[axis] / count - average * average;
    return sqrt(variance > 0.0 ? variance : 0.0);
  }
};

uint8_t sensorAddress = 0;
RunState runState = RunState::kFatal;
bool calibrationValid = false;
bool zuptEnabled = false;
bool zuptActive = false;
bool stationaryCandidate = false;
uint32_t stationarySinceUs = 0;
uint32_t calibrationStartedMs = 0;
uint32_t activeCalibrationDurationMs = kCalibrationDurationMs;
uint32_t lastSampleUs = 0;
uint32_t experimentStartUs = 0;
uint32_t missedSamples = 0;
uint32_t serialDroppedRows = 0;
uint8_t segmentId = 0;
Vector3 accelBaseline = {0.0, 0.0, 0.0};
Vector3 gyroBaseline = {0.0, 0.0, 0.0};
Vector3 latestAccel = {0.0, 0.0, 0.0};
Vector3 latestGyro = {0.0, 0.0, 0.0};
Vector3 latestCorrectedAccel = {0.0, 0.0, 0.0};
MotionIntegrator integrator;
CalibrationStats calibrationStats;
char pendingCsvRow[kCsvBufferSize];
size_t pendingCsvLength = 0;
size_t pendingCsvOffset = 0;

bool readRegister(uint8_t reg, uint8_t* value) {
  Wire.beginTransmission(sensorAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(sensorAddress, static_cast<uint8_t>(1), true) != 1) {
    return false;
  }
  *value = Wire.read();
  return true;
}

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(sensorAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission(true) == 0;
}

bool readSample(RawSample* sample) {
  Wire.beginTransmission(sensorAddress);
  Wire.write(kGyroDataRegister);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(sensorAddress, static_cast<uint8_t>(12), true) != 12) {
    return false;
  }

  int16_t* output[6] = {
      &sample->gyro[0], &sample->gyro[1], &sample->gyro[2],
      &sample->accel[0], &sample->accel[1], &sample->accel[2]};
  for (size_t axis = 0; axis < 6; ++axis) {
    const uint8_t low = Wire.read();
    const uint8_t high = Wire.read();
    *output[axis] = static_cast<int16_t>(
        static_cast<uint16_t>(low) |
        (static_cast<uint16_t>(high) << 8));
  }
  return true;
}

bool findSensor() {
  bool foundAddress = false;
  for (uint8_t address : kCandidateAddresses) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) continue;
    foundAddress = true;
    sensorAddress = address;
    uint8_t chipId = 0;
    if (readRegister(kChipIdRegister, &chipId) && chipId == kExpectedChipId) {
      Serial.printf("# BMI160 addr=0x%02X chip_id=0x%02X\n", address, chipId);
      return true;
    }
    Serial.printf("# I2C device at 0x%02X did not report BMI160 chip ID 0xD1\n",
                  address);
  }
  if (!foundAddress) {
    Serial.println("# ERROR: no I2C response at 0x68 or 0x69");
  }
  return false;
}

bool configureSensor() {
  if (!writeRegister(kCommandRegister, 0xB6)) return false;
  delay(100);

  uint8_t chipId = 0;
  if (!readRegister(kChipIdRegister, &chipId) || chipId != kExpectedChipId) {
    Serial.println("# ERROR: BMI160 chip ID check failed after reset");
    return false;
  }

  // 100 Hz ODR, normal bandwidth; ±2 g and ±250 degrees/s.
  if (!writeRegister(kAccelConfigRegister, 0x28) ||
      !writeRegister(kAccelRangeRegister, 0x03) ||
      !writeRegister(kGyroConfigRegister, 0x28) ||
      !writeRegister(kGyroRangeRegister, 0x03)) {
    return false;
  }

  uint8_t value = 0;
  if (!readRegister(kAccelConfigRegister, &value) || value != 0x28 ||
      !readRegister(kAccelRangeRegister, &value) || value != 0x03 ||
      !readRegister(kGyroConfigRegister, &value) || value != 0x28 ||
      !readRegister(kGyroRangeRegister, &value) || value != 0x03) {
    Serial.println("# ERROR: BMI160 configuration readback failed");
    return false;
  }

  if (!writeRegister(kCommandRegister, 0x11)) return false;
  delay(5);
  if (!writeRegister(kCommandRegister, 0x15)) return false;
  delay(80);

  if (!readRegister(kPmuStatusRegister, &value) || (value & 0x3C) != 0x14) {
    Serial.printf("# ERROR: BMI160 PMU status is 0x%02X, expected accel+gyro normal\n",
                  value);
    return false;
  }
  return true;
}

Vector3 accelToSi(const int16_t raw[3]) {
  return {raw[0] * (kGravity / kAccelCountsPerG),
          raw[1] * (kGravity / kAccelCountsPerG),
          raw[2] * (kGravity / kAccelCountsPerG)};
}

Vector3 gyroToSi(const int16_t raw[3]) {
  constexpr double kRadiansPerDegree = PI / 180.0;
  const double scale =
      kRadiansPerDegree / kGyroCountsPerDegreePerSecond;
  return {raw[0] * scale, raw[1] * scale, raw[2] * scale};
}

double magnitude(const Vector3& value) {
  return sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

void printCsvHeader() {
  Serial.println(
      "timestamp_ms,dt_s,accel_x_raw_lsb,accel_y_raw_lsb,accel_z_raw_lsb,"
      "gyro_x_raw_lsb,gyro_y_raw_lsb,gyro_z_raw_lsb,accel_x_m_s2,"
      "accel_y_m_s2,accel_z_m_s2,gyro_x_rad_s,gyro_y_rad_s,gyro_z_rad_s,"
      "corrected_accel_x_m_s2,corrected_accel_y_m_s2,"
      "corrected_accel_z_m_s2,velocity_x_m_s,velocity_y_m_s,velocity_z_m_s,"
      "displacement_x_m,displacement_y_m,displacement_z_m,calibration_valid,"
      "state_flags,missed_samples,serial_dropped_rows,segment_id");
}

void drainSerialBuffer() {
  if (pendingCsvOffset >= pendingCsvLength) {
    pendingCsvLength = 0;
    pendingCsvOffset = 0;
    return;
  }

  const int available = Serial.availableForWrite();
  if (available <= 0) return;
  const size_t remaining = pendingCsvLength - pendingCsvOffset;
  const size_t chunkSize =
      remaining < static_cast<size_t>(available) ? remaining : available;
  const size_t sent = Serial.write(
      reinterpret_cast<const uint8_t*>(pendingCsvRow + pendingCsvOffset),
      chunkSize);
  pendingCsvOffset += sent;
  if (pendingCsvOffset == pendingCsvLength) {
    pendingCsvLength = 0;
    pendingCsvOffset = 0;
  }
}

void emitRow(uint32_t timestampUs, double dtSeconds, const RawSample& raw,
             const char* stateFlags) {
  if (pendingCsvLength != pendingCsvOffset) {
    ++serialDroppedRows;
    return;
  }
  pendingCsvLength = 0;
  pendingCsvOffset = 0;

  const Vector3& velocity = integrator.velocity();
  const Vector3& displacement = integrator.displacement();
  const int formattedLength = snprintf(
      pendingCsvRow, sizeof(pendingCsvRow),
      "%.3f,%.6f,%d,%d,%d,%d,%d,%d,"
      "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
      "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
      "%.6f,%.6f,%.6f,%u,%s,%lu,%lu,%u\n",
      (timestampUs - experimentStartUs) / 1000.0,
      dtSeconds,
      raw.accel[0], raw.accel[1], raw.accel[2],
      raw.gyro[0], raw.gyro[1], raw.gyro[2],
      latestAccel.x, latestAccel.y, latestAccel.z,
      latestGyro.x, latestGyro.y, latestGyro.z,
      latestCorrectedAccel.x, latestCorrectedAccel.y,
      latestCorrectedAccel.z, velocity.x, velocity.y, velocity.z,
      displacement.x, displacement.y, displacement.z,
      calibrationValid ? 1U : 0U, stateFlags,
      static_cast<unsigned long>(missedSamples),
      static_cast<unsigned long>(serialDroppedRows),
      static_cast<unsigned int>(segmentId));
  if (formattedLength < 0 ||
      static_cast<size_t>(formattedLength) >= sizeof(pendingCsvRow)) {
    ++serialDroppedRows;
    return;
  }
  pendingCsvLength = static_cast<size_t>(formattedLength);
}

void beginCalibration(const char* reason,
                      uint32_t durationMs = kCalibrationDurationMs) {
  calibrationValid = false;
  runState = RunState::kCalibrating;
  calibrationStats.clear();
  integrator.reset();
  zuptActive = false;
  stationaryCandidate = false;
  calibrationStartedMs = millis();
  activeCalibrationDurationMs = durationMs;
  Serial.printf("# CALIBRATION: %s; keep the sensor still and in one orientation\n",
                reason);
}

bool calibrationPassed() {
  const double accelMean[3] = {
      calibrationStats.mean(0), calibrationStats.mean(1),
      calibrationStats.mean(2)};
  const double gyroMean[3] = {
      calibrationStats.mean(3), calibrationStats.mean(4),
      calibrationStats.mean(5)};
  const Vector3 accelAverage = {accelMean[0], accelMean[1], accelMean[2]};
  const Vector3 gyroAverage = {gyroMean[0], gyroMean[1], gyroMean[2]};
  const double accelMagnitude = magnitude(accelAverage);

  bool stable = calibrationStats.count >= kMinimumCalibrationSamples &&
                accelMagnitude >= kMinGravityMagnitude &&
                accelMagnitude <= kMaxGravityMagnitude &&
                magnitude(gyroAverage) <= kMaxGyroMean;
  for (size_t axis = 0; axis < 6; ++axis) {
    const double limit = axis < 3 ? kMaxAccelStdDev : kMaxGyroStdDev;
    if (calibrationStats.standardDeviation(axis) > limit) stable = false;
  }

  if (!stable) {
    Serial.printf(
        "# CALIBRATION_RETRY: samples=%lu |mean_accel|=%.3f m/s^2 "
        "|mean_gyro|=%.4f rad/s; retrying (heuristic, not proof of no motion)\n",
        static_cast<unsigned long>(calibrationStats.count), accelMagnitude,
        magnitude(gyroAverage));
    calibrationStats.clear();
    calibrationStartedMs = millis();
    return false;
  }

  accelBaseline = accelAverage;
  gyroBaseline = gyroAverage;
  calibrationValid = true;
  runState = RunState::kRunning;
  integrator.reset();
  stationaryCandidate = false;
  zuptActive = false;
  Serial.printf(
      "# CALIBRATION_VALID fixed_orientation_only samples=%lu "
      "accel_baseline_m_s2=%.6f,%.6f,%.6f "
      "gyro_baseline_rad_s=%.6f,%.6f,%.6f\n",
      static_cast<unsigned long>(calibrationStats.count), accelBaseline.x,
      accelBaseline.y, accelBaseline.z, gyroBaseline.x, gyroBaseline.y,
      gyroBaseline.z);
  return true;
}

void handleSerialCommands() {
  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command == 'c') {
      beginCalibration("serial recalibration; reset displacement");
    } else if (command == 'C') {
      beginCalibration("10-second serial recalibration; reset displacement",
                       kLongCalibrationDurationMs);
    } else if (command == 'z' || command == 'Z') {
      integrator.reset();
      stationaryCandidate = false;
      zuptActive = false;
      Serial.println("# EVENT: integrator reset; displacement and velocity are zero");
    } else if (command == 'm' || command == 'M') {
      zuptEnabled = !zuptEnabled;
      stationaryCandidate = false;
      zuptActive = false;
      Serial.printf("# EVENT: integration_mode=%s\n",
                    zuptEnabled ? "ZUPT_HEURISTIC" : "RAW_INTEGRATION");
    } else if (command >= '0' && command <= '9') {
      segmentId = static_cast<uint8_t>(command - '0');
      Serial.printf("# EVENT: segment_id=%u\n", segmentId);
    }
  }
}

void processSample(const RawSample& raw, uint32_t timestampUs) {
  latestAccel = accelToSi(raw.accel);
  latestGyro = gyroToSi(raw.gyro);
  const double dtSeconds =
      lastSampleUs == 0 ? 0.0 : (timestampUs - lastSampleUs) / 1000000.0;
  lastSampleUs = timestampUs;

  if (dtSeconds > 1.5 * kNominalPeriodSeconds) {
    const uint32_t expected =
        static_cast<uint32_t>(dtSeconds * kOutputDataRateHz + 0.5);
    if (expected > 1) missedSamples += expected - 1;
  }

  if (runState == RunState::kCalibrating) {
    calibrationStats.add(latestAccel, latestGyro);
    latestCorrectedAccel = {0.0, 0.0, 0.0};
    if (millis() - calibrationStartedMs >= activeCalibrationDurationMs) {
      calibrationPassed();
    }
    emitRow(timestampUs, dtSeconds, raw,
            calibrationValid ? "CALIBRATION_COMPLETE" : "CALIBRATING");
    return;
  }

  latestCorrectedAccel = {
      latestAccel.x - accelBaseline.x,
      latestAccel.y - accelBaseline.y,
      latestAccel.z - accelBaseline.z};

  const Vector3 gyroResidual = {
      latestGyro.x - gyroBaseline.x,
      latestGyro.y - gyroBaseline.y,
      latestGyro.z - gyroBaseline.z};
  const bool stationaryNow =
      magnitude(latestCorrectedAccel) < kStationaryAccelThreshold &&
      magnitude(gyroResidual) < kStationaryGyroThreshold;
  const char* flags = zuptEnabled ? "RUNNING_ZUPT_HEURISTIC" : "RUNNING_RAW";

  if (zuptEnabled && stationaryNow) {
    if (!stationaryCandidate) {
      stationaryCandidate = true;
      stationarySinceUs = timestampUs;
    }
    if (timestampUs - stationarySinceUs >= kStationaryHoldMs * 1000UL) {
      zuptActive = true;
      integrator.holdStationary(latestCorrectedAccel);
      flags = "RUNNING_ZUPT_HEURISTIC|STATIONARY_CANDIDATE|ZUPT_APPLIED";
    } else {
      zuptActive = false;
      flags = "RUNNING_ZUPT_HEURISTIC|STATIONARY_CANDIDATE";
    }
  } else {
    stationaryCandidate = false;
    zuptActive = false;
  }

  if (runState == RunState::kRunning && calibrationValid &&
      !(zuptEnabled && zuptActive) && dtSeconds > 0.0) {
    if (!integrator.step(latestCorrectedAccel, dtSeconds)) {
      Serial.println("# FATAL: invalid integration timestep; integration stopped");
      runState = RunState::kFatal;
      return;
    }
  }
  emitRow(timestampUs, dtSeconds, raw, flags);
}

}  // namespace

void setup() {
  Serial.begin(921600);
  Wire.begin(kSdaPin, kSclPin);
  Wire.setClock(100000);
  Wire.setTimeOut(50);
  Serial.printf("# BMI160 motion experiment; SDA=GPIO%u SCL=GPIO%u I2C=100kHz\n",
                kSdaPin, kSclPin);
  Serial.println("# Sensor setup scans 0x68/0x69, checks chip ID/config and PMU.");

  if (!findSensor() || !configureSensor()) {
    Serial.println("# FATAL: sensor initialization failed; no motion integration");
    runState = RunState::kFatal;
    return;
  }

  experimentStartUs = micros();
  Serial.println("# CSV values: acceleration m/s^2, gyro rad/s, distance m.");
  Serial.println("# 100 Hz; accel ±2g; gyro ±250 deg/s; no orientation fusion.");
  Serial.println("# Baseline subtraction is FIXED_ORIENTATION_ONLY, not 3D gravity removal.");
  Serial.println("# Commands: c=2s recalibrate/reset, C=10s recalibrate/reset,");
  Serial.println("# z=zero velocity+displacement,");
  Serial.println("# m=toggle RAW_INTEGRATION/ZUPT heuristic, 0-9=set segment_id.");
  Serial.println("# Calibration gates are heuristic; motion can pass or constant velocity look still.");
  printCsvHeader();
  beginCalibration("startup; after sensor settling, collect at least 2 seconds");
}

void loop() {
  if (runState == RunState::kFatal) return;
  drainSerialBuffer();
  handleSerialCommands();

  uint8_t status = 0;
  if (!readRegister(kStatusRegister, &status)) {
    Serial.println("# FATAL: I2C status read failed; integration stopped");
    runState = RunState::kFatal;
    return;
  }
  if ((status & kDataReadyMask) != kDataReadyMask) return;

  RawSample raw;
  if (!readSample(&raw)) {
    Serial.println("# FATAL: I2C sample read failed; integration stopped");
    runState = RunState::kFatal;
    return;
  }
  processSample(raw, micros());
  drainSerialBuffer();
}
