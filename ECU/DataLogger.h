#pragma once

#include "Arduino.h"
#include "FS.h"
#include "SD_MMC.h"
#include "cppQueue.h"
#include "ArduinoJson.h"

struct Datum {
  unsigned long delta_time; 
  float current_angle; 
  float dpu; 
  int rpm; 
  int coil_state; 
  float target_angle; 
  bool rolled_over;
  float predicted_angle;
  bool predicted;
};

class DataLogger {
private:
  cppQueue *stream;
  char csv[64];
  char buff[10];
  volatile uint8_t state;

  Datum pop;
  unsigned long capture_timestamp;
  unsigned long capture_period;

  File file;

public:
  static const uint8_t READY = 0;
  static const uint8_t CAPTURING = 1;
  static const uint8_t COMPLETE = 2;

public:
  DataLogger();
  bool begin(DynamicJsonDocument config);
  void startCapture();
  void update();
  void log(unsigned long delta_time, float current_angle, float dpu, int rpm, int coil_state, float target_angle, bool rolled_over, float predicted_angle, bool predicted);
  uint8_t getState();
};
