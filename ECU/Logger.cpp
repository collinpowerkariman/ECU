#include "Logger.h"

Logger_ &Logger_::getInstance() {
  static Logger_ instance;
  return instance;
}

Logger_ &Logger = Logger.getInstance();

bool Logger_::begin() {
  // mounting sd card
  log("Mounting sd card: ");
  if (!SD_MMC.begin("/sdcard", true)) {
    logln("Failed");
    return false;
  }
  logln("Success");

  // check for logs directory
  log("Logger -> Checking logs directory: ");
  File dir = SD_MMC.open("/logs");
  if (!dir) {
    logln("Not exists");
    log("Creating logs directory: ");
    if (!SD_MMC.mkdir("/logs")) {
      logln("Failed");
      return false;
    }
    logln("Success");
  } else if (dir && !dir.isDirectory()) {
    logln("Not a directory");
    return false;
  } else {
    logln("Exists");
  }

  // create log file
  log("Creating log file: ");
  for (int i = 0; i < 100; i++) {
    String name = "/logs/log-" + String(i) + ".txt";
    if (!SD_MMC.exists(name)) {
      file = SD_MMC.open(name, FILE_WRITE);
      if (!file) {
        logln("Failed to create " + name);
        return false;
      } else {
        logln("Successfully create " + name);
      }
      break;
    }
  }

  return true;
}

void Logger_::log(String msg) {
  Serial.print(msg);
  if (file) {
    file.print(msg);
    file.flush();
  }
}

void Logger_::logln(String msg) {
  Serial.println(msg);
  if (file) {
    file.println(msg);
    file.flush();
  }
}
