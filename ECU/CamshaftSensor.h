#pragma once

#include "Arduino.h"
#include "AS5047P.h"
#include "ArduinoJson.h"

#define CAMSHAFT_SENSOR_SPI_CS_PIN 5
#define CAMSHAFT_SENSOR_SPI_BUS_SPEED 800000
#define CAMSHAFT_SENSOR_ZERO_BTN_PIN 34

class CamshaftSensor_ {
private:
  CamshaftSensor_() = default;

public:
  static CamshaftSensor_ &getInstance();
  CamshaftSensor_(const CamshaftSensor_ &) = delete;
  CamshaftSensor_ &operator=(const CamshaftSensor_ &) = delete;

private:
  AS5047P *sensor;

  unsigned long current_timestamp;
  unsigned long previous_timestamp;

  unsigned long delta_time;

  float zero_position;
  float current_angle;
  float previous_angle;
  bool rolled_over;
  float predicted_angle;
  bool predicted_angle_used;

  float readings[10];
  int index;
  float sum;
  float dpu;
  float acc;
  int rpm;

  bool updateAngle();

public:
  struct Reading {
    unsigned long elapsed;
    float angle;
    float dpu;
    int rpm;
    bool rolled_over;
    float predicted_angle;
    bool predicted_angle_used;
  };

public:
  bool begin(DynamicJsonDocument config);
  Reading update();
  bool zero();
};

extern CamshaftSensor_ &CamshaftSensor;
