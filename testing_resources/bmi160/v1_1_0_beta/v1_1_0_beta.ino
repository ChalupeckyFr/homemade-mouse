#include <Arduino.h>
#include <Wire.h>
#include <math.h>

constexpr uint8_t IMU = 0x69;
constexpr int SDA_PIN = 20;
constexpr int SCL_PIN = 10;

constexpr float ACC_SCALE = 9.80665f / 16384.0f;
constexpr float GYR_SCALE = 1.0f / 131.2f;
constexpr float RAD_PER_DEG = 0.01745329252f;

// 100 Hz: 20 vzorku je priblizne 200 ms.
constexpr int WINDOW = 20;
constexpr float QUIET_CONFIRM_S = 0.30f;
constexpr float ENTER_ACC_MEAN = 0.040f;
constexpr float ENTER_ACC_NOISE = 0.045f;
constexpr float ENTER_GYRO_RMS = 0.35f;
constexpr float EXIT_ACC_INSTANT = 0.100f;
constexpr float EXIT_ACC_MEAN = 0.060f;
constexpr float EXIT_GYRO_INSTANT = 0.60f;
constexpr float EXIT_ACC_NOISE = 0.070f;
constexpr int MAX_SAMPLES = 1200;

struct Sample {
  float t, dt;
  int16_t raw[6];       // gx, gy, gz, ax, ay, az
  float a[3];           // pred potlacenim klidu, m/s^2
  float v[3];           // po pripadne korekci, m/s
  float p[3];           // m
  float meanAcc;
  float noiseAcc;
  float gyroRms;
  uint8_t rest;
};

Sample samples[MAX_SAMPLES];
int sampleCount = 0;
float gyroBias[3] = {};
float restAccel[3] = {};
float velocity[3] = {};
float positionM[3] = {};
float previousA[3] = {};
float qw = 1, qx = 0, qy = 0, qz = 0;

struct WindowItem {
  float a[3];
  float gyroSquared;
};

WindowItem history[WINDOW];
int historyCount = 0, historyIndex = 0;
float meanAcc = 0, noiseAcc = 0, gyroRms = 0;
float quietTime = 0;
bool resting = true;

float norm3(const float v[3]) {
  return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(IMU);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readRegs(uint8_t reg, uint8_t *data, size_t count) {
  Wire.beginTransmission(IMU);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(IMU, count) != count) return false;
  for (size_t i = 0; i < count; ++i) data[i] = Wire.read();
  return true;
}

bool readFresh(int16_t raw[6], uint32_t &stamp) {
  uint32_t start = millis();
  while (uint32_t(millis() - start) < 100) {
    uint8_t status;
    if (!readRegs(0x1B, &status, 1)) return false;
    if ((status & 0xC0) == 0xC0) {
      uint8_t bytes[12];
      if (!readRegs(0x0C, bytes, 12)) return false;
      stamp = micros();
      for (int i = 0; i < 6; ++i) {
        uint16_t word = uint16_t(bytes[2*i]) |
                        (uint16_t(bytes[2*i+1]) << 8);
        raw[i] = int16_t(word);
      }
      return true;
    }
    delay(1);
  }
  return false;
}

bool initIMU() {
  uint8_t id;
  if (!readRegs(0x00, &id, 1) || id != 0xD1) return false;
  if (!writeReg(0x7E, 0xB6)) return false;
  delay(100);
  if (!writeReg(0x7E, 0x11)) return false;
  delay(10);
  if (!writeReg(0x7E, 0x15)) return false;
  delay(100);
  if (!writeReg(0x40, 0x28)) return false;
  if (!writeReg(0x41, 0x03)) return false;
  if (!writeReg(0x42, 0x28)) return false;
  if (!writeReg(0x43, 0x03)) return false;
  delay(100);
  uint8_t config[4], pmu;
  if (!readRegs(0x40, config, 4)) return false;
  if (!readRegs(0x03, &pmu, 1)) return false;
  return config[0] == 0x28 && config[1] == 0x03 &&
         config[2] == 0x28 && config[3] == 0x03 &&
         (pmu & 0x3C) == 0x14;
}

void updateRotation(const float gyro[3], float dt) {
  float gx = gyro[0] * RAD_PER_DEG;
  float gy = gyro[1] * RAD_PER_DEG;
  float gz = gyro[2] * RAD_PER_DEG;
  float w = qw, x = qx, y = qy, z = qz;
  float h = 0.5f * dt;
  qw += (-x*gx - y*gy - z*gz) * h;
  qx += ( w*gx + y*gz - z*gy) * h;
  qy += ( w*gy - x*gz + z*gx) * h;
  qz += ( w*gz + x*gy - y*gx) * h;
  float n = sqrtf(qw*qw + qx*qx + qy*qy + qz*qz);
  qw /= n;
  qx /= n;
  qy /= n;
  qz /= n;
}

void rotateToStart(const float in[3], float out[3]) {
  float tx = 2 * (qy*in[2] - qz*in[1]);
  float ty = 2 * (qz*in[0] - qx*in[2]);
  float tz = 2 * (qx*in[1] - qy*in[0]);
  out[0] = in[0] + qw*tx + qy*tz - qz*ty;
  out[1] = in[1] + qw*ty + qz*tx - qx*tz;
  out[2] = in[2] + qw*tz + qx*ty - qy*tx;
}

bool calibrate() {
  Serial.println("\nKALIBRACE: 3 sekundy se sestavy nedotykej.");
  Serial.flush();
  int16_t raw[6];
  uint32_t stamp;
  if (!readFresh(raw, stamp)) return false;
  double mean[6] = {};
  double m2[6] = {};
  for (int n = 1; n <= 300; ++n) {
    if (!readFresh(raw, stamp)) return false;
    for (int i = 0; i < 6; ++i) {
      double value = raw[i] * (i < 3 ? GYR_SCALE : ACC_SCALE);
      double delta = value - mean[i];
      mean[i] += delta / n;
      m2[i] += delta * (value - mean[i]);
    }
  }
  float gyroVariance = 0, accelVariance = 0;
  for (int i = 0; i < 3; ++i) {
    gyroBias[i] = mean[i];
    restAccel[i] = mean[i+3];
    gyroVariance += m2[i] / 299.0;
    accelVariance += m2[i+3] / 299.0;
  }
  float gyroNoise = sqrtf(gyroVariance);
  float accelNoise = sqrtf(accelVariance);
  Serial.printf("Gyro bias [deg/s]: %.5f, %.5f, %.5f\n",
                gyroBias[0], gyroBias[1], gyroBias[2]);
  Serial.printf("Klidovy vektor [m/s2]: %.5f, %.5f, %.5f\n",
                restAccel[0], restAccel[1], restAccel[2]);
  Serial.printf("Kalibracni sum accel/gyro: %.5f / %.5f\n",
                accelNoise, gyroNoise);
  float g = norm3(restAccel);
  if (g < 8.5f || g > 11.0f ||
      accelNoise > 0.045f || gyroNoise > 0.25f) {
    Serial.println("Kalibrace nestabilni. Pokus nezahajen.");
    return false;
  }
  return true;
}

void updateWindow(const float a[3], const float gyro[3]) {
  WindowItem &item = history[historyIndex];
  for (int i = 0; i < 3; ++i) item.a[i] = a[i];
  item.gyroSquared =
    gyro[0]*gyro[0] + gyro[1]*gyro[1] + gyro[2]*gyro[2];
  historyIndex = (historyIndex + 1) % WINDOW;
  if (historyCount < WINDOW) ++historyCount;
  float mean[3] = {};
  float gyroSum = 0;
  for (int n = 0; n < historyCount; ++n) {
    for (int i = 0; i < 3; ++i) mean[i] += history[n].a[i];
    gyroSum += history[n].gyroSquared;
  }
  for (int i = 0; i < 3; ++i) mean[i] /= historyCount;
  float varianceSum = 0;
  for (int n = 0; n < historyCount; ++n) {
    for (int i = 0; i < 3; ++i) {
      float d = history[n].a[i] - mean[i];
      varianceSum += d*d;
    }
  }
  meanAcc = norm3(mean);
  noiseAcc = sqrtf(varianceSum / historyCount);
  gyroRms = sqrtf(gyroSum / historyCount);
}

void updateRestDetector(const float a[3], const float gyro[3], float dt) {
  bool activity =
    norm3(a) > EXIT_ACC_INSTANT ||
    norm3(gyro) > EXIT_GYRO_INSTANT ||
    (historyCount == WINDOW &&
      (meanAcc > EXIT_ACC_MEAN || noiseAcc > EXIT_ACC_NOISE));
  bool quiet =
    historyCount == WINDOW &&
    meanAcc < ENTER_ACC_MEAN &&
    noiseAcc < ENTER_ACC_NOISE &&
    gyroRms < ENTER_GYRO_RMS;
  if (activity) {
    resting = false;
    quietTime = 0;
  } else if (resting) {
    quietTime = 0;
  } else if (quiet) {
    quietTime += dt;
    if (quietTime >= QUIET_CONFIRM_S) resting = true;
  } else {
    quietTime = 0;
  }
}

void printSummary(bool failed) {
  Serial.println("\n=== VYSLEDEK | v1.1.0-beta ===");
  if (failed) Serial.println("POZOR: chyba cteni nebo casovani.");
  Serial.printf("Pocet vzorku: %d\n", sampleCount);
  if (!sampleCount) return;
  int restCount = 0, transitions = 0, clippedCount = 0;
  float maxDt = 0, path = 0;
  float prevP[3] = {};
  for (int n = 0; n < sampleCount; ++n) {
    const Sample &s = samples[n];
    if (s.rest) ++restCount;
    if (n && s.rest != samples[n-1].rest) ++transitions;
    if (s.dt > maxDt) maxDt = s.dt;
    float step[3];
    for (int i = 0; i < 3; ++i) {
      step[i] = s.p[i] - prevP[i];
      prevP[i] = s.p[i];
    }
    path += norm3(step);
    bool clipped = false;
    for (int i = 0; i < 6; ++i) {
      if (s.raw[i] > 32700 || s.raw[i] < -32700) clipped = true;
    }
    if (clipped) ++clippedCount;
  }
  const Sample &last = samples[sampleCount-1];
  Serial.printf("Delka mereni: %.3f s\n", last.t);
  Serial.printf("Max interval: %.3f ms\n", maxDt*1000);
  Serial.printf("Vzorky oznacene KLID: %.1f %%\n",
                100.0f*restCount/sampleCount);
  Serial.printf("Zmeny stavu: %d\n", transitions);
  Serial.printf("Koncovy stav: %s\n", last.rest ? "KLID" : "POHYB");
  Serial.printf("Posun X/Y/Z [cm]: %.3f, %.3f, %.3f\n",
                last.p[0]*100, last.p[1]*100, last.p[2]*100);
  Serial.printf("Rychlost X/Y/Z [cm/s]: %.3f, %.3f, %.3f\n",
                last.v[0]*100, last.v[1]*100, last.v[2]*100);
  Serial.printf("Odhad delky drahy [cm]: %.3f\n", path*100);
  Serial.printf("Vzorky blizko limitu rozsahu: %d\n", clippedCount);
  Serial.println("r = korekce; d = CSV; s = novy pokus (prepise zaznam).");
}

void runTrial() {
  sampleCount = 0;
  if (!calibrate()) {
    Serial.println("Zkontroluj klid a spojeni, potom posli s.");
    return;
  }
  qw = 1;
  qx = qy = qz = 0;
  historyCount = historyIndex = 0;
  meanAcc = noiseAcc = gyroRms = quietTime = 0;
  resting = true;
  for (int i = 0; i < 3; ++i) {
    velocity[i] = positionM[i] = previousA[i] = 0;
  }
  while (Serial.available()) Serial.read();
  Serial.println("\nSTART - TED MUZES POHYBOVAT.");
  Serial.println("k = konec; automaticky konec nejpozdeji za 12 s.");
  Serial.flush();
  int16_t raw[6];
  uint32_t previousStamp;
  if (!readFresh(raw, previousStamp)) {
    Serial.println("CHYBA: nelze zahajit zaznam.");
    return;
  }
  uint32_t wallStart = millis();
  float elapsed = 0;
  bool failed = false;
  while (sampleCount < MAX_SAMPLES &&
         uint32_t(millis() - wallStart) < 12000) {
    bool stop = false;
    while (Serial.available()) {
      char c = Serial.read();
      if (c == 'k' || c == 'K') stop = true;
    }
    if (stop) break;
    uint32_t stamp;
    if (!readFresh(raw, stamp)) {
      failed = true;
      break;
    }
    float dt = uint32_t(stamp - previousStamp)*0.000001f;
    previousStamp = stamp;
    if (dt <= 0 || dt > 0.05f) {
      failed = true;
      break;
    }
    elapsed += dt;
    float gyro[3], bodyA[3], worldA[3], linearA[3];
    for (int i = 0; i < 3; ++i) {
      gyro[i] = raw[i]*GYR_SCALE - gyroBias[i];
      bodyA[i] = raw[i+3]*ACC_SCALE;
    }
    updateRotation(gyro, dt);
    rotateToStart(bodyA, worldA);
    for (int i = 0; i < 3; ++i) {
      linearA[i] = worldA[i] - restAccel[i];
    }
    updateWindow(linearA, gyro);
    updateRestDetector(linearA, gyro, dt);
    for (int i = 0; i < 3; ++i) {
      if (resting) {
        velocity[i] = 0;
        previousA[i] = 0;
      } else {
        float oldV = velocity[i];
        velocity[i] += 0.5f*(previousA[i] + linearA[i])*dt;
        positionM[i] += 0.5f*(oldV + velocity[i])*dt;
        previousA[i] = linearA[i];
      }
    }
    Sample &s = samples[sampleCount++];
    s.t = elapsed;
    s.dt = dt;
    s.rest = resting;
    s.meanAcc = meanAcc;
    s.noiseAcc = noiseAcc;
    s.gyroRms = gyroRms;
    for (int i = 0; i < 6; ++i) s.raw[i] = raw[i];
    for (int i = 0; i < 3; ++i) {
      s.a[i] = linearA[i];
      s.v[i] = velocity[i];
      s.p[i] = positionM[i];
    }
  }
  printSummary(failed);
}

void dumpSamples() {
  if (!sampleCount) {
    Serial.println("Zatim zadny zaznam.");
    return;
  }
  Serial.println("Verze: v1.1.0-beta");
  Serial.println("CSV_BEGIN");
  Serial.println(
    "t_s,dt_ms,rest,"
    "gx_raw,gy_raw,gz_raw,ax_raw,ay_raw,az_raw,"
    "gx_dps,gy_dps,gz_dps,"
    "ax_linear_mss,ay_linear_mss,az_linear_mss,"
    "mean_acc_mss,noise_acc_mss,gyro_rms_dps,"
    "vx_cms,vy_cms,vz_cms,px_cm,py_cm,pz_cm"
  );
  for (int n = 0; n < sampleCount; ++n) {
    const Sample &s = samples[n];
    Serial.printf("%.4f,%.3f,%u", s.t, s.dt*1000, unsigned(s.rest));
    for (int i = 0; i < 6; ++i) {
      Serial.printf(",%d", int(s.raw[i]));
    }
    for (int i = 0; i < 3; ++i) {
      Serial.printf(",%.5f", s.raw[i]*GYR_SCALE - gyroBias[i]);
    }
    for (int i = 0; i < 3; ++i) Serial.printf(",%.5f", s.a[i]);
    Serial.printf(",%.5f,%.5f,%.5f",
                  s.meanAcc, s.noiseAcc, s.gyroRms);
    for (int i = 0; i < 3; ++i) Serial.printf(",%.5f", s.v[i]*100);
    for (int i = 0; i < 3; ++i) Serial.printf(",%.5f", s.p[i]*100);
    Serial.println();
    if (n % 20 == 0) delay(1);
  }
  Serial.println("CSV_END");
}

void printCorrectedResult() {
  Serial.println("\n=== ZPETNA KOREKCE | v1.1.0-beta ===");
  Serial.println("Predpoklad: kazdy usek zacina a konci v klidu.");
  Serial.println("Puvodni data ani vysledek se nemeni.");
  if (!sampleCount) {
    Serial.println("Zatim zadny zaznam.");
    return;
  }

  float correctedTotal[3] = {};
  int completedSegments = 0;
  bool incomplete = false;
  int n = 0;
  while (n < sampleCount) {
    while (n < sampleCount && samples[n].rest) ++n;
    if (n == sampleCount) break;
    int first = n;
    while (n < sampleCount && !samples[n].rest) ++n;
    if (n == sampleCount) {
      Serial.println("Nedokonceny usek: na konci nebyl potvrzen klid.");
      incomplete = true;
      break;
    }
    if (first == 0) {
      Serial.println("Usek preskocen: chybi predchozi klidovy vzorek.");
      incomplete = true;
      continue;
    }

    const Sample &before = samples[first - 1];
    const Sample &end = samples[n - 1];
    float duration = end.t - before.t;
    if (duration <= 0.0f) {
      Serial.println("Usek preskocen: neplatna delka.");
      incomplete = true;
      continue;
    }
    float originalDelta[3], correctedDelta[3], estimatedBias[3];
    for (int axis = 0; axis < 3; ++axis) {
      originalDelta[axis] = end.p[axis] - before.p[axis];
      correctedDelta[axis] =
        originalDelta[axis] - 0.5f * end.v[axis] * duration;
      estimatedBias[axis] = end.v[axis] / duration;
      correctedTotal[axis] += correctedDelta[axis];
    }
    ++completedSegments;
    Serial.printf("\nUsek %d\n", completedSegments);
    Serial.printf("Cas integrovaneho useku: %.4f az %.4f s\n",
                  before.t, end.t);
    Serial.printf("Detektor potvrdil klid v: %.4f s\n", samples[n].t);
    Serial.printf("Delka useku: %.4f s\n", duration);
    Serial.printf("Puvodni posun X/Y/Z [cm]: %.3f, %.3f, %.3f\n",
                  originalDelta[0]*100, originalDelta[1]*100,
                  originalDelta[2]*100);
    Serial.printf("Rychlost pred nulovanim [cm/s]: %.3f, %.3f, %.3f\n",
                  end.v[0]*100, end.v[1]*100, end.v[2]*100);
    Serial.printf("Modelovana chyba zrychleni [m/s2]: %.5f, %.5f, %.5f\n",
                  estimatedBias[0], estimatedBias[1], estimatedBias[2]);
    Serial.printf("Opraveny posun X/Y/Z [cm]: %.3f, %.3f, %.3f\n",
                  correctedDelta[0]*100, correctedDelta[1]*100,
                  correctedDelta[2]*100);
  }

  Serial.printf("\nDokoncene vyhodnocene useky: %d\n", completedSegments);
  if (completedSegments) {
    Serial.printf("Soucet opravenych posunu X/Y/Z [cm]: %.3f, %.3f, %.3f\n",
                  correctedTotal[0]*100, correctedTotal[1]*100,
                  correctedTotal[2]*100);
    Serial.printf("Velikost vysledneho posunu [cm]: %.3f\n",
                  norm3(correctedTotal)*100);
    Serial.println("To neni delka cele ujete drahy.");
  } else {
    Serial.println("Zadny dokonceny pohybovy usek k oprave.");
  }
  if (incomplete) {
    Serial.println("POZOR: vysledek nezahrnuje vsechny useky.");
  }
  Serial.println("Oprava neni dukaz presnosti; over ji znamou vzdalenosti.");
}

void printVersion() {
  Serial.println("\n==================================");
  Serial.println("BMI160 EXPERIMENT | v1.1.0-beta");
  Serial.println("ESP32-C3 | SDA 20 | SCL 10 | IMU 0x69");
  Serial.println("s = start; k = konec; d = CSV; r = korekce; v = verze");
  Serial.println("==================================");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  printVersion();
  if (!Wire.begin(SDA_PIN, SCL_PIN, 100000)) {
    Serial.println("CHYBA I2C.");
    while (true) delay(1000);
  }
  Wire.setTimeOut(50);
  if (!initIMU()) {
    Serial.println("CHYBA BMI160 nebo jeho nastaveni.");
    while (true) delay(1000);
  }
  Serial.println("BMI160: experiment s detekci klidu, 100 Hz.");
  Serial.println("Dalsi s prepise predchozi zaznam.");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S') runTrial();
    if (c == 'd' || c == 'D') dumpSamples();
    if (c == 'r' || c == 'R') printCorrectedResult();
    if (c == 'v' || c == 'V') printVersion();
  }
  delay(1);
}
