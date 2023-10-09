#include "DataLogger.h"
#include "Logger.h"

DataLogger::DataLogger() {
  capture_timestamp = 0;
  state = COMPLETE;
}

bool DataLogger::begin(DynamicJsonDocument config) {
  Logger.logln("Loading data logger configuration... ");
  capture_period = config["data_logger"]["capture_period"];
  Logger.logln("flush_interval: " + String(capture_period));
  int stream_size = config["data_logger"]["stream_size"];
  Logger.logln("stream_size: " + String(stream_size));
  Logger.logln("Load Complete.");

  stream = new cppQueue(sizeof(Datum), stream_size);
  state = READY;

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

  return true;
}

void DataLogger::startCapture() {
  if (state != READY) {
    return;
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
        file.println("time_delta,current_angle,dpu,rpm,coil_state,target_angle,rolled_over,predicted_angle,predicted");
      }
      break;
    }
  }

  capture_timestamp = micros();
  state = CAPTURING;
}

void DataLogger::update() {
  if (state != CAPTURING) {
    return;
  }

  if ((micros() - capture_timestamp) >= capture_period) {
    state = COMPLETE;
  }

  if (stream->pop(&pop)) {
    // clean previous log
    csv[0] = 0;
    buff[0] = 0;

    // log delta time
    ltoa(pop.delta_time, buff, 10);
    strcat(csv, buff);

    // log current angle
    strcat(csv, ",");
    dtostrf(pop.current_angle, 7, 3, buff);
    strcat(csv, buff);

    // log current dpu
    strcat(csv, ",");
    dtostrf(pop.dpu, 7, 5, buff);
    strcat(csv, buff);

    // log current rpm
    strcat(csv, ",");
    ltoa(pop.rpm, buff, 10);
    strcat(csv, buff);

    // log coil state
    strcat(csv, ",");
    ltoa(pop.coil_state, buff, 10);
    strcat(csv, buff);

    // log coil target angle
    strcat(csv, ",");
    dtostrf(pop.target_angle, 5, 1, buff);
    strcat(csv, buff);

    // log rolled over
    strcat(csv, ",");
    if (pop.rolled_over) {
      strcat(csv, "1");
    } else {
      strcat(csv, "0");
    }

    // log predicted angle
    strcat(csv, ",");
    dtostrf(pop.predicted_angle, 7, 3, buff);
    strcat(csv, buff);

    // log predicted
    strcat(csv, ",");
    if (pop.predicted) {
      strcat(csv, "1");
    } else {
      strcat(csv, "0");
    }

    file.println(csv);
  }

  if (state == COMPLETE) {
    file.flush();
    file.close();
    state = READY;
  }
}

void DataLogger::log(unsigned long delta_time, float current_angle, float dpu, int rpm, int coil_state, float target_angle, bool rolled_over, float predicted_angle, bool predicted) {
  if (state != CAPTURING) {
    return;
  }

  if (coil_state == 0 && !rolled_over) {
    return;
  }

  Datum datum = { delta_time, current_angle, dpu, rpm, coil_state, target_angle, rolled_over, predicted_angle, predicted };
  stream->push(&datum);
}

uint8_t DataLogger::getState() {
  return state;
}
