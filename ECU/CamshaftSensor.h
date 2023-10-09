#pragma once

#include "Arduino.h"
#include "AS5047P.h"
#include "ArduinoJson.h"

#define CAMSHAFT_SENSOR_SPI_CS_PIN 5
#define CAMSHAFT_SENSOR_SPI_BUS_SPEED 800000
#define CAMSHAFT_SENSOR_ZERO_BTN_PIN 34

class CamshaftSensor {
private:
  AS5047P sensor;
  
  unsigned long current_timestamp;
  unsigned long previous_timestamp;

  unsigned long delta_time;

  float zero_position;
  float current_angle;
  float previous_angle;
  float predicted_angle;
  bool predicted;
  bool rolled_over;

  float readings[10];
  int index;
  float sum;
  float dpu;
  float acc;
  int rpm;

public:
  CamshaftSensor();
  bool begin(DynamicJsonDocument config);
  bool update();
  bool zero();
  bool isRolledOver();
  bool isPredicted();
  unsigned long getDeltaTime();
  float getCurrentAngle();
  float getPreviousAngle();
  float getPredictedAngle();
  float getDPU();
  int getRPM();
};
