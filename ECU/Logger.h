#pragma once

#include "Arduino.h"
#include "SD_MMC.h"

class Logger_ {
private:
  Logger_() = default;

public:
  static Logger_& getInstance();
  Logger_(const Logger_ &) = delete;
  Logger_ &operator=(const Logger_ &) = delete;

private:
  File file;

public:
  bool begin();
  void log(String msg);
  void logln(String msg);

};

extern Logger_ &Logger;
