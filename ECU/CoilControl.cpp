#include "esp32-hal.h"
#include "CoilControl.h"
#include "SD_MMC.h"
#include "FS.h"
#include "Logger.h"

CoilControl::CoilControl() {}

void CoilControl::updateTargetAngle() {
  if (state != PRIMED) {
    return;
  }

  int step = (rpm / 100) - 1;
  if (step > 39) {
    step = 39;
  } else if (step < 0) {
    step = 0;
  }

  target_angle = target_angle_lut[step];
}

bool CoilControl::shouldDischarge() {
  if (state != CHARGING) {
    return false;
  }

  if (current_angle >= target_angle) {
    return true;
  }

  unsigned long charge_time = current_timestamp - charging_timestamp;
  return (charge_time >= maximum_charge_time);
}

void CoilControl::discharge() {
  digitalWrite(COIL_PIN, LOW);
  state = DISCHARGED;
}

bool CoilControl::shouldCharge() {
  if (state != PRIMED) {
    return false;
  }

  if (current_angle >= target_angle) {
    return false;
  }

  if (rpm < minimum_rpm) {
    return false;
  }

  float available = target_angle - current_angle;
  float required = dpu * minimum_charge_time;
  return (available <= required);
}

void CoilControl::charge() {
  digitalWrite(COIL_PIN, HIGH);
  state = CHARGING;
  charging_timestamp = micros();
}

bool CoilControl::shouldPrime() {
  if (state == CHARGING) {
    return false;
  }

  return rolled_over;
}

void CoilControl::prime() {
  digitalWrite(COIL_PIN, LOW);
  state = PRIMED;
}

bool CoilControl::begin(DynamicJsonDocument config) {
  Logger.logln("Loading coil control configuration... ");
  
  minimum_charge_time = config["coil_control"]["minimum_charge_time"];
  Logger.logln("minimum_charge_time: " + String(minimum_charge_time));
  
  maximum_charge_time = config["coil_control"]["maximum_charge_time"];
  Logger.logln("maximum_charge_time: " + String(maximum_charge_time));

  minimum_rpm = config["coil_control"]["minimum_rpm"];
  Logger.logln("minimum_rpm: " + String(minimum_rpm));

  for (int i = 0; i < 40; i++) {
    int step = (i + 1) * 100;
    target_angle_lut[i] = config["coil_control"]["target_angle_lut"][String(step)];
    Logger.logln("target_angle_lut[" + String(i) + "]: " + String(target_angle_lut[i]));
  }
  Logger.logln("Load Complete.");

  pinMode(COIL_PIN, OUTPUT);
  prime();
  return true;
}

bool IRAM_ATTR CoilControl::update(bool rolled_over, float current_angle, float dpu, int rpm) {
  this->rolled_over = rolled_over;
  this->current_angle = current_angle;
  this->dpu = dpu;
  this->rpm = rpm;
  
  current_timestamp = micros();
  updateTargetAngle();

  if (shouldDischarge()) {
    discharge();
  } else if (shouldCharge()) {
    charge();
  } else if (shouldPrime()) {
    prime();
  }

  return true;
}

int CoilControl::getState() {
  return state;
}

float CoilControl::getTargetAngle() {
  return target_angle;
}
