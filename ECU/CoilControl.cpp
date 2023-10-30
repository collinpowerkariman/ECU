#include "esp32-hal.h"
#include "CoilControl.h"
#include "SD_MMC.h"
#include "FS.h"
#include "Logger.h"

CoilControl_ &CoilControl_::getInstance() {
  static CoilControl_ instance;
  return instance;
}

CoilControl_ &CoilControl = CoilControl.getInstance();

void CoilControl_::updateTargetAngle() {
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

bool CoilControl_::shouldDischarge() {
  if (state != CHARGING) {
    return false;
  }

  if (angle >= target_angle) {
    return true;
  }

  unsigned long charge_time = current_timestamp - charging_timestamp;
  return (charge_time >= maximum_charge_time);
}

void CoilControl_::discharge() {
  digitalWrite(COIL_PIN, LOW);
  state = DISCHARGED;
}

bool CoilControl_::shouldCharge() {
  if (state != PRIMED) {
    return false;
  }

  if (angle >= target_angle) {
    return false;
  }

  if (rpm < minimum_rpm) {
    return false;
  }

  float available = target_angle - angle;
  float required = dpu * minimum_charge_time;
  return (available <= required);
}

void CoilControl_::charge() {
  digitalWrite(COIL_PIN, HIGH);
  state = CHARGING;
  charging_timestamp = micros();
  charging_rpm = rpm;
}

bool CoilControl_::shouldPrime() {
  if (state == CHARGING) {
    return false;
  }

  return rolled_over;
}

void CoilControl_::prime() {
  digitalWrite(COIL_PIN, LOW);
  state = PRIMED;
  charging_rpm = 0;
}

bool CoilControl_::begin(DynamicJsonDocument config) {
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

CoilControl_::Reading IRAM_ATTR CoilControl_::update(bool rolled_over, float angle, float dpu, int rpm) {
  this->rolled_over = rolled_over;
  this->angle = angle;
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

  return {state, charging_rpm, target_angle};
}
