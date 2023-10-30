#include "FS.h"
#include "SD_MMC.h"
#include "CamshaftSensor.h"
#include "CoilControl.h"
#include "RawDataLogger.h"
#include "DataLogger.h"
#include "Logger.h"
#include "soc/rtc_wdt.h"
#include "esp_task_wdt.h"

TaskHandle_t MainThread;
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

  if (!CamshaftSensor.begin(config)) {
    while (true) continue;
  }

  if (!CoilControl.begin(config)) {
    while (true) continue;
  }

  if (!RawDataLogger.begin(config)) {
    while (true) continue;
  }

  if (!DataLogger.begin()) {
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

  CamshaftSensor_::Reading camshaft;
  CoilControl_::Reading coil;
  for (;;) {
    camshaft = CamshaftSensor.update();
    coil = CoilControl.update(camshaft.rolled_over, camshaft.angle, camshaft.dpu, camshaft.rpm);
    RawDataLogger.log(camshaft.elapsed, camshaft.angle, camshaft.dpu, camshaft.rpm, camshaft.rolled_over, camshaft.predicted_angle, camshaft.predicted_angle_used, coil.state, coil.charging_rpm, coil.target_discharge_angle);
    delayMicroseconds(30);
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
    if (digitalRead(34) == LOW) {
      RawDataLogger.toggle();
    }
    RawDataLogger.update();
  }
}


void loop() {
}
