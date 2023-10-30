#pragma once

#include "Arduino.h"

class DataLogger_ {
private:
  DataLogger_() = default;

public:
  static DataLogger_ &getInstance();
  DataLogger_(const DataLogger_ &) = delete;
  DataLogger_ &operator=(const DataLogger_ &) = delete;

private:
  int readings_rpm[5];
  float readings_angle[5];
  int readings_index;
  int readings_rpm_sum;
  bool readings_filled;

  int previous_coil_state;
  int rpm_step;
  float discharge_angle;
  int discharge_rpm;
  float gain_angle;

  uint8_t state;
  static const uint8_t READY = 0;
  static const uint8_t ACCEPTING = 1;
  static const uint8_t COMPLETED = 2;

  void reset();
  void complete();

public:
  bool begin();
  void update();
  void log(float angle, int rpm, int current_coil_state);
};

extern DataLogger_ &DataLogger;