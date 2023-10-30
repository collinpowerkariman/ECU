#include "DataLogger.h"
#include "CoilControl.h"
#include "Logger.h"

DataLogger_ &DataLogger_::getInstance() {
  static DataLogger_ instance;
  return instance;
}

DataLogger_ &DataLogger = DataLogger.getInstance();

void DataLogger_::reset() {
  for (int i = 0; i < 5; i++) {
    readings_rpm[i] = 0;
    readings_angle[i] = 0;
  }
  readings_index = 0;
  readings_rpm_sum = 0;
  readings_filled = false;

  rpm_step = 0;
  discharge_angle = 0;
  discharge_rpm = 0;
  gain_angle = 0;

  state = READY;
}

void DataLogger_::complete() {
  state = COMPLETED;
}

bool DataLogger_::begin() {
  reset();
  previous_coil_state = CoilControl_::DISCHARGED;
  Logger.logln(F("Data Logger Ready"));
  return true;
}

void DataLogger_::update() {
  if (state != COMPLETED) {
    return;
  }

  if (rpm_step > 0) {
    Logger.logln("rpm_step: " + String(rpm_step) + ", discharge_angle: " + String(discharge_angle) + ", discharge_rpm: " + String(discharge_rpm) + ", gain_angle: " + String(gain_angle));
  }

  reset();
}

void DataLogger_::log(float angle, int rpm, int current_coil_state) {
  if (state == READY && previous_coil_state == CoilControl_::DISCHARGED && current_coil_state == CoilControl_::PRIMED) {
    state = ACCEPTING;
  }

  if (state != ACCEPTING) {
    previous_coil_state = current_coil_state;
    return;
  }

  if (previous_coil_state == CoilControl_::PRIMED && current_coil_state == CoilControl_::CHARGING) {
    rpm_step = (rpm / 100) * 100;
    previous_coil_state = current_coil_state;
    return;
  }

  if (current_coil_state != CoilControl_::DISCHARGED) {
    previous_coil_state = current_coil_state;
    return;
  }

  if (previous_coil_state == CoilControl_::CHARGING) {
    discharge_angle = angle;
    discharge_rpm = rpm;
    previous_coil_state = current_coil_state;
    return;
  }

  readings_angle[readings_index] = angle;
  readings_rpm_sum -= readings_rpm[readings_index];
  readings_rpm[readings_index] = (rpm - discharge_rpm);
  readings_rpm_sum += readings_rpm[readings_index];

  if (readings_index == 4) {
    readings_filled = true;
  }

  if (readings_filled && (readings_rpm_sum / 5) > 5) {
    int tail_index = readings_index - 4;
    if (tail_index < 0) {
      tail_index = readings_index + 1;
    }
    gain_angle = readings_angle[tail_index];
    complete();
    return;
  }

  if (readings_index >= 4) {
    readings_index = 0;
  } else {
    readings_index++;
  }
  previous_coil_state = current_coil_state;
}
