#pragma once

#include <packet.h>

typedef struct {
  // types: DELAY_DETECT, SENSOR_DETECT
  packet_t packet;
  // Polymorphic details.
  // if DELAY_DETECT, ticks waited
  // if SENSOR_DETECT, sensor detected
  int details;
  // Unique identifier for the detector type, returned on creation.
  // Helps identify what you might have detected (e.g. delays)
  int identifier;
} detector_message_t;