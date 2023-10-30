#include "RawDataLogger.h"
#include "Logger.h"

RawDataLogger_ &RawDataLogger_::getInstance() {
  static RawDataLogger_ instance;
  return instance;
}

RawDataLogger_ &RawDataLogger = RawDataLogger.getInstance();

bool RawDataLogger_::begin(DynamicJsonDocument config) {
  Logger.logln("Loading data logger configuration... ");
  // capture_period = config["data_logger"]["capture_period"];
  // Logger.logln("flush_interval: " + String(capture_period));
  // int stream_size = config["data_logger"]["stream_size"];
  // Logger.logln("stream_size: " + String(stream_size));
  Logger.logln("Load Complete.");

  // check for data directory
  Logger.log("Checking data directory: ");
  File dir = SD_MMC.open("/data");
  if (!dir) {
    Logger.logln("Not exists");
    Logger.log("Creating logs directory: ");
    if (!SD_MMC.mkdir("/data")) {
      Logger.logln("Failed");
      return false;
    }
    Logger.logln("Success");
  } else if (dir && !dir.isDirectory()) {
    Logger.logln("Not a directory");
    return false;
  } else {
    Logger.logln("Exists");
  }

  capture_state = CAPTURE_STOPPED;
  data_state = DATA_PROCESS_COMPLETED;
  buffer_idx = 0;
  buffer_size = 1000;
  return true;
}

void RawDataLogger_::toggle() {
  if (capture_state == CAPTURE_STOPPED) {
    startCapture();
  } else {
    stopCapture();
  }
  delay(1000);
}

void RawDataLogger_::startCapture() {
  if (capture_state == CAPTURE_RUNNING || data_state != DATA_PROCESS_COMPLETED) {
    return;
  }

  if (file) {
    file.close();
  }

  // create data file
  Logger.log("Creating data file: ");
  for (int i = 0; i < 100; i++) {
    String name = "/data/data-" + String(i) + ".csv";
    if (!SD_MMC.exists(name)) {
      file = SD_MMC.open(name, FILE_WRITE);
      if (!file) {
        Logger.logln("Failed to create " + name);
        return;
      } else {
        Logger.logln("Successfully create " + name);
        file.println("elapsed,camshaft_angle,camshaft_dpu,camshaft_rpm,camshaft_rolled_over,camshaft_predicted_angle,camshaft_predicted_angle_used,coil_state,coil_charging_rpm,coil_target_discharge_angle,kendall's_tau");
      }
      break;
    }
  }

  buffer_idx = 0;
  capture_state = CAPTURE_RUNNING;
}

void RawDataLogger_::stopCapture() {
  capture_state = CAPTURE_STOPPED;

  while (data_state != DATA_PROCESS_COMPLETED) {
    update();
  }

  file.flush();
  file.close();
}

void RawDataLogger_::update() {
  if (data_state != DATA_CAPTURE_COMPLETED) {
    return;
  }

  data_state = DATA_PROCESS_RUNNING;
  char csv[256];
  char buff[10];
  Datum datum;
  int concordant;
  int disconcordant;
  float correlation;
  int base_rpm = buffer[0].camshaft_rpm;
  for (int i = 0; i < buffer_idx - 5; i++) {
    concordant = 0;
    disconcordant = 0;

    concordant += (buffer[i+0].camshaft_rpm - base_rpm) < (buffer[i+1].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+0].camshaft_rpm - base_rpm) < (buffer[i+2].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+0].camshaft_rpm - base_rpm) < (buffer[i+3].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+0].camshaft_rpm - base_rpm) < (buffer[i+4].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+1].camshaft_rpm - base_rpm) < (buffer[i+2].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+1].camshaft_rpm - base_rpm) < (buffer[i+3].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+1].camshaft_rpm - base_rpm) < (buffer[i+4].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+2].camshaft_rpm - base_rpm) < (buffer[i+3].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+2].camshaft_rpm - base_rpm) < (buffer[i+4].camshaft_rpm - base_rpm) ? 1 : 0;
    concordant += (buffer[i+3].camshaft_rpm - base_rpm) < (buffer[i+4].camshaft_rpm - base_rpm) ? 1 : 0;

    disconcordant += (buffer[i+0].camshaft_rpm - base_rpm) > (buffer[i+1].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+0].camshaft_rpm - base_rpm) > (buffer[i+2].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+0].camshaft_rpm - base_rpm) > (buffer[i+3].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+0].camshaft_rpm - base_rpm) > (buffer[i+4].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+1].camshaft_rpm - base_rpm) > (buffer[i+2].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+1].camshaft_rpm - base_rpm) > (buffer[i+3].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+1].camshaft_rpm - base_rpm) > (buffer[i+4].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+2].camshaft_rpm - base_rpm) > (buffer[i+3].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+2].camshaft_rpm - base_rpm) > (buffer[i+4].camshaft_rpm - base_rpm) ? 1 : 0;
    disconcordant += (buffer[i+3].camshaft_rpm - base_rpm) > (buffer[i+4].camshaft_rpm - base_rpm) ? 1 : 0;

    correlation = float(concordant-disconcordant)/float(concordant+disconcordant);
    if (correlation < 0.8) {
      continue;
    }
    
    datum = buffer[i];
    csv[0] = 0;
    buff[0] = 0;

    ltoa(datum.elapsed, buff, 10);
    strcat(csv, buff);

    strcat(csv, ",");
    dtostrf(datum.camshaft_angle, 7, 3, buff);
    strcat(csv, buff);

    strcat(csv, ",");
    dtostrf(datum.camshaft_dpu, 7, 5, buff);
    strcat(csv, buff);

    strcat(csv, ",");
    ltoa(datum.camshaft_rpm, buff, 10);
    strcat(csv, buff);

    strcat(csv, ",");
    if (datum.camshaft_rolled_over) {
      strcat(csv, "1");
    } else {
      strcat(csv, "0");
    }

    strcat(csv, ",");
    dtostrf(datum.camshaft_predicted_angle, 7, 3, buff);
    strcat(csv, buff);

    strcat(csv, ",");
    if (datum.camshaft_predicted_angle_used) {
      strcat(csv, "1");
    } else {
      strcat(csv, "0");
    }

    strcat(csv, ",");
    ltoa(datum.coil_state, buff, 10);
    strcat(csv, buff);

    strcat(csv, ",");
    ltoa(datum.coil_charging_rpm, buff, 10);
    strcat(csv, buff);

    strcat(csv, ",");
    dtostrf(datum.coil_target_discharge_angle, 7, 3, buff);
    strcat(csv, buff);

    strcat(csv, ",");
    dtostrf(correlation, 7, 3, buff);
    strcat(csv, buff);

    file.println(csv);
    break;
  }
  
  buffer_idx = 0;
  data_state = DATA_PROCESS_COMPLETED;
}

void RawDataLogger_::log(unsigned long elapsed, float camshaft_angle, float camshaft_dpu, int camshaft_rpm, bool camshaft_rolled_over, float camshaft_predicted_angle, bool camshaft_predicted_angle_used, int coil_state, int coil_charging_rpm, float coil_target_discharge_angle) {
  if (data_state == DATA_PROCESS_COMPLETED && capture_state == CAPTURE_RUNNING && camshaft_rolled_over) {
    data_state = DATA_CAPTURE_RUNNING;
  }

  if (data_state != DATA_CAPTURE_RUNNING) {
    return;
  }

  if (camshaft_angle < 182) {
    return;
  }

  buffer[buffer_idx] = { elapsed,
                         camshaft_angle,
                         camshaft_dpu,
                         camshaft_rpm,
                         camshaft_rolled_over,
                         camshaft_predicted_angle,
                         camshaft_predicted_angle_used,
                         coil_state,
                         coil_charging_rpm,
                         coil_target_discharge_angle };
  buffer_idx = buffer_idx + 1;

  if (buffer_idx >= buffer_size || camshaft_angle > 269) {
    data_state = DATA_CAPTURE_COMPLETED;
  }
}
