#pragma once

#include "Arduino.h"
#include "FS.h"
#include "SD_MMC.h"
#include "ArduinoJson.h"

class RawDataLogger_ {
private:
  RawDataLogger_() = default;

public:
  static RawDataLogger_ &getInstance();
  RawDataLogger_(const RawDataLogger_ &) = delete;
  RawDataLogger_ &operator=(const RawDataLogger_ &) = delete;

private:
  File file;

  struct Datum {
    unsigned long elapsed;
    float camshaft_angle;
    float camshaft_dpu;
    int camshaft_rpm;
    bool camshaft_rolled_over;
    float camshaft_predicted_angle;
    bool camshaft_predicted_angle_used;
    int coil_state;
    int coil_charging_rpm;
    float coil_target_discharge_angle;
  };
  Datum buffer[1000];
  uint8_t buffer_size = 1000;
  uint8_t buffer_idx;

  volatile uint8_t data_state;
  static const uint8_t DATA_PROCESS_RUNNING = 0;
  static const uint8_t DATA_PROCESS_COMPLETED = 1;
  static const uint8_t DATA_CAPTURE_RUNNING = 2;
  static const uint8_t DATA_CAPTURE_COMPLETED = 3;

  volatile uint8_t capture_state;
  static const uint8_t CAPTURE_RUNNING = 0;
  static const uint8_t CAPTURE_STOPPED = 1;

  void startCapture();
  void stopCapture();
public:
  bool begin(DynamicJsonDocument config);
  void toggle();
  void update();
  void log(unsigned long elapsed, float camshaft_angle, float camshaft_dpu, int camshaft_rpm, bool camshaft_rolled_over, float camshaft_predicted_angle, bool camshaft_predicted_angle_used, int coil_state, int coil_charging_rpm, float coil_target_discharge_angle);
};

extern RawDataLogger_ &RawDataLogger;
