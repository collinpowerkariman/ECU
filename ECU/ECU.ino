#include "FS.h"
#include "SD_MMC.h"
#include "CamshaftSensor.h"
#include "CoilControl.h"
#include "DataLogger.h"
#include "Logger.h"
#include "soc/rtc_wdt.h"
#include "esp_task_wdt.h"

CamshaftSensor camshaftSensor;
CoilControl coilControl;
TaskHandle_t MainThread;

DataLogger dataLogger;
TaskHandle_t SubThread;

void setup() {
  Serial.begin(115200);
  while (!Serial) continue;

  if (!Logger.begin()) {
    while (true) continue;
  }

  // reading configuration file of sdcard
  Logger.log("Loading configuration: ");
  File f = SD_MMC.open("/configuration.json", FILE_READ, true);
  DynamicJsonDocument config(2048);
  if (!f) {
    Logger.logln("Failed");
    while (true) continue;
  } else {
    deserializeJson(config, f.readString());
    Logger.logln("Success");
  }
  f.close();

  if (!camshaftSensor.begin(config)) {
    while (true) continue;
  }

  if (!coilControl.begin(config)) {
    while (true) continue;
  }

  if (!dataLogger.begin(config)) {
    while (true) continue;
  }

  xTaskCreatePinnedToCore(MainLoop, "MainThread", 4096, NULL, configMAX_PRIORITIES, &MainThread, 0);
  xTaskCreatePinnedToCore(SubLoop, "SubThread", 4096, NULL, configMAX_PRIORITIES, &SubThread, 1);
}

void MainLoop(void *p) {
  rtc_wdt_protect_off();
  rtc_wdt_disable();
  disableCore0WDT();
  disableLoopWDT();
  esp_task_wdt_delete(NULL);

  unsigned long delta_time;
  bool rolled_over;
  bool predicted;
  float current_angle;
  float predicted_angle;
  float dpu;
  int rpm;
  int coil_state;
  float target_angle;

  for (;;) {
    camshaftSensor.update();
    delta_time = camshaftSensor.getDeltaTime();
    rolled_over = camshaftSensor.isRolledOver();
    current_angle = camshaftSensor.getCurrentAngle();
    dpu = camshaftSensor.getDPU();
    rpm = camshaftSensor.getRPM();
    predicted_angle = camshaftSensor.getPredictedAngle();
    predicted = camshaftSensor.isPredicted();

    coilControl.update(rolled_over, current_angle, dpu, rpm);
    coil_state = coilControl.getState();
    target_angle = coilControl.getTargetAngle();

    dataLogger.log(delta_time, current_angle, dpu, rpm, coil_state, target_angle, rolled_over, predicted_angle, predicted);
    delayMicroseconds(34);
  }
}

void SubLoop(void *p) {
  rtc_wdt_protect_off();
  rtc_wdt_disable();
  disableCore1WDT();
  disableLoopWDT();
  esp_task_wdt_delete(NULL);

  pinMode(34, INPUT);

  for (;;) {
    if (dataLogger.getState() == DataLogger::READY) {
      if (digitalRead(34) == LOW) {
        dataLogger.startCapture();
      }
    }
    dataLogger.update();
  }
}


void loop() {
}
