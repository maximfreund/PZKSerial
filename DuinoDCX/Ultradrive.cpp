#include "Ultradrive.h"

const byte Ultradrive::vendorHeader[5] = {0xF0, 0x00, 0x20, 0x32, 0x00};

Ultradrive::Ultradrive(HardwareSerial *serial, int rtsPin, int ctsPin)
    : serial(serial),
      rtsPin(rtsPin),
      ctsPin(ctsPin),
      flowControl(false),
      selectedDevice(0),
      devices(),
      dump0(),
      dump1(),
      pingResponse(),
      hasDump0(false),
      hasDump1(false),
      invalidateSync(false),
      isFirstRun(true),
      lastSearch(0),
      lastPing(0),
      lastResync(0),
      unansweredPings(0),
      readingCommand(false),
      serialRead(0),
      serialBuffer(),
      serverBuffer() {}

void Ultradrive::enableFlowControl(bool enabled) {
  flowControl = enabled;
}

// -----------------------------------------------------------------------------
//  Polling loop
// -----------------------------------------------------------------------------

void Ultradrive::processIncoming(unsigned long now) {
  while (serial->available() > 0) {
    readCommands(now);
  }

  // At most one outgoing request per call keeps the RS-232 line calm.
  if (isFirstRun) {
    isFirstRun = false;
    lastSearch = now;
    search();
    return;
  }

  if (now - lastSearch >= SEARCH_INTERVAL) {
    Serial.printf("%lu: Searching for devices\n", now);
    lastSearch = now;
    search();
    return;
  }

  if (now - lastPing >= PING_INTERVAL) {
    Serial.printf("%lu: Pinging device %d\n", now, selectedDevice);
    lastPing = now;
    if (unansweredPings < 255) {
      unansweredPings++;
    }
    ping(selectedDevice);
    return;
  }

  if (now - lastResync >= RESYNC_INTERVAL &&
      isFresh(devices[selectedDevice].lastResponse, now, TIMEOUT_TIME)) {
    Serial.printf("%lu: Syncing device %d\n", now, selectedDevice);
    lastResync = now;
    setTransmitMode(selectedDevice);
    dump(selectedDevice, 0);
    dump(selectedDevice, 1);
  }
}

// -----------------------------------------------------------------------------
//  Web API helpers
// -----------------------------------------------------------------------------

void Ultradrive::writeDevice(Response *res) {
  // Only hand out a complete and current snapshot. Never a half-filled one
  // (right after boot) or the previously selected device's state.
  if (hasDump0 && hasDump1 && isFresh(lastResync, millis(), TIMEOUT_TIME)) {
    res->write(dump0, PART_0_LENGTH);
    res->write(dump1, PART_1_LENGTH);
  }
}

void Ultradrive::writeDeviceStatus(Response *res) {
  if (!isFresh(lastPing, millis(), TIMEOUT_TIME)) {
    return;
  }

  // The device stopped answering: show silence instead of frozen meters.
  if (unansweredPings > MAX_UNANSWERED_PINGS) {
    memset(pingResponse, 0, sizeof(pingResponse));
  }

  res->write(pingResponse, PING_RESPONSE_LENGTH);
}

void Ultradrive::writeDevices(Response *res) {
  unsigned long now = millis();

  for (int i = 0; i < MAX_DEVICES; i++) {
    if (isFresh(devices[i].lastResponse, now, TIMEOUT_TIME)) {
      res->write(devices[i].response, SEARCH_RESPONSE_LENGTH);
    }
  }
}

bool Ultradrive::setSelected(int deviceId) {
  if (deviceId < 0 || deviceId >= MAX_DEVICES) {
    return false;
  }

  if (deviceId != selectedDevice) {
    selectedDevice = deviceId;
    // Forget everything we know about the previous device and resync now.
    hasDump0 = false;
    hasDump1 = false;
    invalidateSync = false;
    lastResync = 0;
    unansweredPings = 0;
    memset(pingResponse, 0, sizeof(pingResponse));
  }

  return true;
}

int Ultradrive::getSelected() {
  return selectedDevice;
}

void Ultradrive::processOutgoing(Request *req) {
  // Leave room for the terminator that readBytesUntil() swallows.
  int length = req->readBytesUntil(TERMINATOR, serverBuffer, PART_0_LENGTH - 1);
  if (length <= 0) {
    return;
  }
  serverBuffer[length++] = TERMINATOR;

  if (length <= COMMAND_BYTE ||
      memcmp(serverBuffer, vendorHeader, sizeof(vendorHeader)) != 0 ||
      serverBuffer[COMMAND_BYTE] != DIRECT_COMMAND) {
    return;  // only parameter changes are forwarded to the device
  }

  invalidateSync = true;  // a dump already in flight is now outdated
  applyCommands(serverBuffer, length);
  write(serverBuffer, length);
}

// -----------------------------------------------------------------------------
//  RS-232 output
// -----------------------------------------------------------------------------

size_t Ultradrive::write(const uint8_t *buffer, size_t size) {
  size_t written = 0;

  if (requestToSend(CTS_TIMEOUT)) {
    written = serial->write(buffer, size);
  }

  endSend();

  return written;
}

bool Ultradrive::requestToSend(unsigned long timeout) {
  if (!flowControl) {
    return true;
  }

  unsigned long start = millis();
  digitalWrite(rtsPin, HIGH);

  while (millis() - start <= timeout) {
    if (digitalRead(ctsPin) == HIGH) {
      return true;
    }
  }

  return false;
}

void Ultradrive::endSend() {
  if (flowControl) {
    digitalWrite(rtsPin, LOW);
  }
}

void Ultradrive::search() {
  byte command[] = {0xF0, 0x00, 0x20, 0x32, 0x20, 0x0E, 0x40, TERMINATOR};
  write(command, sizeof(command));
}

void Ultradrive::setTransmitMode(int deviceId) {
  byte command[] = {0xF0, 0x00, 0x20, 0x32, (byte)deviceId, 0x0E, 0x3F, 0x0C, 0x00, TERMINATOR};
  write(command, sizeof(command));
}

void Ultradrive::ping(int deviceId) {
  byte command[] = {0xF0, 0x00, 0x20, 0x32, (byte)deviceId, 0x0E, 0x44, 0x00, 0x00, TERMINATOR};
  write(command, sizeof(command));
}

void Ultradrive::dump(int deviceId, int part) {
  byte command[] = {0xF0, 0x00, 0x20, 0x32, (byte)deviceId, 0x0E, 0x50, 0x01, 0x00, (byte)part, TERMINATOR};
  write(command, sizeof(command));
}

// -----------------------------------------------------------------------------
//  RS-232 input
// -----------------------------------------------------------------------------

void Ultradrive::readCommands(unsigned long now) {
  byte b = serial->read();

  if (b == COMMAND_START) {
    readingCommand = true;
    serialRead = 0;
  }

  if (!readingCommand || serialRead >= PART_0_LENGTH) {
    readingCommand = false;  // noise between frames, or an oversized frame
    serialRead = 0;
    return;
  }

  serialBuffer[serialRead++] = b;

  if (b == TERMINATOR) {
    readingCommand = false;
    handleFrame(now);
  }
}

void Ultradrive::handleFrame(unsigned long now) {
  if (serialRead <= COMMAND_BYTE || memcmp(serialBuffer, vendorHeader, sizeof(vendorHeader)) != 0) {
    return;
  }

  switch (serialBuffer[COMMAND_BYTE]) {
    case SEARCH_RESPONSE: {
      int deviceId = serialBuffer[ID_BYTE];
      if (serialRead == SEARCH_RESPONSE_LENGTH && deviceId < MAX_DEVICES) {
        Serial.printf("%lu: Found device %d\n", now, deviceId);
        devices[deviceId].lastResponse = now;
        memcpy(devices[deviceId].response, serialBuffer, SEARCH_RESPONSE_LENGTH);
      }
      break;
    }

    case DUMP_RESPONSE: {
      if (serialRead <= PART_BYTE) {
        break;
      }

      int part = serialBuffer[PART_BYTE];
      if (part == 0) {
        if (invalidateSync) {
          break;  // requested before the last parameter change: outdated
        }
        if (serialRead == PART_0_LENGTH) {
          Serial.printf("%lu: Received state part 0\n", now);
          memcpy(dump0, serialBuffer, PART_0_LENGTH);
          hasDump0 = true;
        }
      } else if (part == 1) {
        if (invalidateSync) {
          invalidateSync = false;
          break;
        }
        if (serialRead == PART_1_LENGTH) {
          Serial.printf("%lu: Received state part 1\n", now);
          memcpy(dump1, serialBuffer, PART_1_LENGTH);
          hasDump1 = true;
        }
      }
      break;
    }

    case PING_RESPONSE: {
      if (serialRead == PING_RESPONSE_LENGTH) {
        memcpy(pingResponse, serialBuffer, PING_RESPONSE_LENGTH);
        unansweredPings = 0;
      }
      break;
    }

    case DIRECT_COMMAND: {
      Serial.printf("%lu: Parameter change from device\n", now);
      applyCommands(serialBuffer, serialRead);
      break;
    }

    default:
      break;
  }
}

// -----------------------------------------------------------------------------
//  Keeping the cached dump in sync
// -----------------------------------------------------------------------------

void Ultradrive::applyCommands(const byte *frame, int length) {
  // Never trust the count byte alone: only walk entries that are actually
  // inside the frame (minus the trailing terminator).
  int available = (length - CHANNEL_BYTE - 1) / PARAM_ENTRY_SIZE;
  int count = frame[PARAM_COUNT_BYTE];
  if (count > available) {
    count = available;
  }

  for (int i = 0; i < count; i++) {
    const byte *entry = frame + i * PARAM_ENTRY_SIZE;
    const DataLocation *location = locate(entry[CHANNEL_BYTE], entry[PARAM_BYTE]);

    if (location) {
      patchBuffer(entry[VALUE_LOW_BYTE], entry[VALUE_HI_BYTE], *location);
    }
  }
}

const DataLocation *Ultradrive::locate(int channel, int param) {
  if (channel == 0) {
    if (param >= 2 && param <= 11) {
      return &setupLocations[param - 2];
    }
    if (param >= 20 && param <= 24) {
      return &setupLocations[param - 10];
    }
    return nullptr;
  }

  int index = param - 2;

  if (channel >= 1 && channel <= INPUT_CHANNELS) {
    return (index >= 0 && index < INPUT_PARAMS) ? &inputLocations[channel - 1][index] : nullptr;
  }

  if (channel > INPUT_CHANNELS && channel <= INPUT_CHANNELS + OUTPUT_CHANNELS) {
    return (index >= 0 && index < OUTPUT_PARAMS)
               ? &outputLocations[channel - INPUT_CHANNELS - 1][index]
               : nullptr;
  }

  return nullptr;
}

byte *Ultradrive::partBuffer(int part) {
  if (part == 0) {
    return dump0;
  }
  if (part == 1) {
    return dump1;
  }
  return nullptr;
}

void Ultradrive::patchBuffer(int low, int high, const DataLocation &location) {
  if (byte *buffer = partBuffer(location.low.part)) {
    buffer[location.low.byte] = low;
  }

  if (location.middle.byte > 0) {
    if (byte *buffer = partBuffer(location.middle.part)) {
      if (high & 1) {
        buffer[location.middle.byte] |= (1u << location.middle.index);
      } else {
        buffer[location.middle.byte] &= ~(1u << location.middle.index);
      }
    }
  }

  if (location.high.byte > 0) {
    if (byte *buffer = partBuffer(location.high.part)) {
      buffer[location.high.byte] = high >> 1;
    }
  }
}

bool Ultradrive::isFresh(unsigned long timestamp, unsigned long now, unsigned long timeout) {
  return timestamp && now - timestamp < timeout;
}
