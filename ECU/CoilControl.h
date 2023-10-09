#pragma once

#include "Arduino.h"
#include "ArduinoJson.h"

class CoilControl {
private:
  static const int COIL_PIN = 16;

  int state;
  unsigned long current_timestamp;
  unsigned long charging_timestamp;
  int minimum_charge_time;
  int maximum_charge_time;

  bool rolled_over;
  float current_angle;
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

  CoilControl();
  bool begin(DynamicJsonDocument config);
  bool update(bool rolled_over, float current_angle, float dpu, int rpm);
  int getState();
  float getTargetAngle();
};
