#include "CamshaftSensor.h"
#include "SD_MMC.h"
#include "FS.h"
#include "Logger.h"

CamshaftSensor::CamshaftSensor()
  : sensor(CAMSHAFT_SENSOR_SPI_CS_PIN, CAMSHAFT_SENSOR_SPI_BUS_SPEED) {
}

bool CamshaftSensor::begin(DynamicJsonDocument config) {
  // loading configuration
  Logger.logln("Loading camshaft configuration... ");
  zero_position = config["camshaft"]["zero_position"];
  Logger.logln("zero_position: " + String(zero_position));
  Logger.logln("Load Complete.");

  // initialize spi connection
  Logger.log("Connecting to camshaft sensor: ");
  if (!sensor.initSPI()) {
    Logger.logln("Failed");
    return false;
  } else {
    Logger.logln("Success");
  }

  pinMode(CAMSHAFT_SENSOR_ZERO_BTN_PIN, INPUT);
  if (digitalRead(CAMSHAFT_SENSOR_ZERO_BTN_PIN) == LOW) {
    zero();
    delay(1000);
  }

  for (int i = 0; i < 10; i++) {
    readings[i] = 0;
  }
  sum = 0;
  dpu = 0;
  rpm = 0;
  acc = 0;

  current_timestamp = micros();
  current_angle = sensor.readAngleDegree();
  if (current_angle >= zero_position) {
    current_angle = current_angle - zero_position;
  } else {
    current_angle = 360.0 + (current_angle - zero_position);
  }
  predicted_angle = 0;

  return true;
}

bool IRAM_ATTR CamshaftSensor::update() {
  current_timestamp = micros();

  // applying zero position
  current_angle = sensor.readAngleDegree();
  if (current_angle >= zero_position) {
    current_angle = current_angle - zero_position;
  } else {
    current_angle = 360.0 + (current_angle - zero_position);
  }

  // detecting rollover, since update() will be called very often
  // sufficiently large delta can be consider as sign of a rollover.
  float delta_angle = abs(current_angle - previous_angle);
  if (delta_angle > 270.0) {
    delta_angle = 360.0 - delta_angle;
    rolled_over = true;
  } else {
    rolled_over = false;
  }

  // calculate velocity
  delta_time = current_timestamp - previous_timestamp;

  // reject outlier, potentially cause by the slack on the camshaft drive chain.
  // replace oulier with predcted angle using d=v*t+0.5*a*t^2
  float reading = delta_angle / delta_time;
  predicted_angle = previous_angle + (dpu * delta_time) + (0.5 * acc * delta_time * delta_time);
  if (previous_angle >= 180 && previous_angle < 270 && abs(current_angle - predicted_angle) > 90) {
    current_angle = predicted_angle;
    delta_angle = abs(current_angle - previous_angle);
    reading = delta_angle / delta_time;
    predicted = true;
  } else {
    predicted = false;
  }

  if (index == 9) {
    index = 0;
  } else {
    index++;
  }

  sum = sum - readings[index];
  readings[index] = reading;
  sum = sum + readings[index];
  acc = ((sum / 10.0) - dpu) / delta_time;
  dpu = sum / 10.0;
  rpm = dpu * 60000000 / 360;

  previous_timestamp = current_timestamp;
  previous_angle = current_angle;
  return true;
}

bool CamshaftSensor::zero() {
  zero_position = sensor.readAngleDegree();

  Logger.log("Saving camshaft zero position: ");
  File f = SD_MMC.open("/configuration.json", FILE_WRITE);
  if (!f) {
    Logger.logln("Failed");
    return false;
  } else {
    DynamicJsonDocument config(2048);
    deserializeJson(config, f.readString());
    config["camshaft"]["zero_position"] = zero_position;

    String output;
    serializeJson(config, output);

    f.print(output);
    f.flush();
    f.close();
    Logger.logln("Success");
  }
  return true;
}

bool CamshaftSensor::isRolledOver() {
  return rolled_over;
}

bool CamshaftSensor::isPredicted() {
  return predicted;
}

float CamshaftSensor::getCurrentAngle() {
  return current_angle;
}

float CamshaftSensor::getPreviousAngle() {
  return previous_angle;
}

float CamshaftSensor::getPredictedAngle() {
  return predicted_angle;
}

float CamshaftSensor::getDPU() {
  return dpu;
}

int CamshaftSensor::getRPM() {
  return rpm;
}

unsigned long CamshaftSensor::getDeltaTime() {
  return delta_time;
}
