#pragma once

#include "Arduino.h"
#include "ArduinoJson.h"

class CoilControl_ {
private:
  CoilControl_() = default;

public:
  static CoilControl_ &getInstance();
  CoilControl_(const CoilControl_ &) = delete;
  CoilControl_ &operator=(const CoilControl_ &) = delete;

private:
  static const int COIL_PIN = 16;

  int state;
  unsigned long current_timestamp;
  unsigned long charging_timestamp;
  int minimum_charge_time;
  int maximum_charge_time;
  int charging_rpm;

  bool rolled_over;
  float angle;
  float target_angle;
  float target_angle_lut[40];

  float dpu;
  int rpm;
  int minimum_rpm;

  void updateTargetAngle();
  bool shouldDischarge();
  void discharge();
  bool shouldCharge();
  void charge();
  bool shouldPrime();
  void prime();

public:
  static const int PRIMED = 0;
  static const int CHARGING = 1;
  static const int DISCHARGED = 2;
  struct Reading {
    int state;
    int charging_rpm;
    float target_discharge_angle;
  };

public:
  bool begin(DynamicJsonDocument config);
  Reading update(bool rolled_over, float angle, float dpu, int rpm);
};

extern CoilControl_ &CoilControl;
