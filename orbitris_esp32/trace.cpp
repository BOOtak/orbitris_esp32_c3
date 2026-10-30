#include "trace.h"

#include <Arduino.h>

void do_print(const char *msg) {
  Serial.print(msg);
}

uint32_t platform_micros() {
  return micros();
}
