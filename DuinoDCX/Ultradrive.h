// =============================================================================
//  Ultradrive – talks to one or more Behringer DCX2496 units over RS-232.
//
//  The DCX speaks a SysEx-style protocol: F0 00 20 32 <id> 0E <cmd> … F7.
//  We periodically search for devices, ping the selected one for its level
//  meters and pull a full state dump (two parts) that the web UI renders.
//  Parameter changes – from the UI or from the knobs on the unit itself – are
//  patched straight into the cached dump so the UI stays in sync in between.
// =============================================================================
#ifndef ULTRADRIVE_H_
#define ULTRADRIVE_H_

#include <Arduino.h>
#include "aWOT.h"

// ---- Tunables (override by #defining before this header) -------------------
#ifndef MAX_DEVICES
#define MAX_DEVICES 16
#endif

#ifndef PING_INTERVAL
#define PING_INTERVAL 1000    // ms between level meter polls
#endif

#ifndef SEARCH_INTERVAL
#define SEARCH_INTERVAL 5000  // ms between device searches
#endif

#ifndef RESYNC_INTERVAL
#define RESYNC_INTERVAL 5000  // ms between full state dumps
#endif

#ifndef TIMEOUT_TIME
#define TIMEOUT_TIME 20000    // ms without an answer before a device counts as gone
#endif

#ifndef MAX_UNANSWERED_PINGS
#define MAX_UNANSWERED_PINGS 3  // level meters drop to zero after this many missed pings
#endif

#ifndef CTS_TIMEOUT
#define CTS_TIMEOUT 1000      // ms to wait for CTS when flow control is enabled
#endif

// ---- Protocol: frame lengths ----------------------------------------------
#define SEARCH_RESPONSE_LENGTH 26
#define PING_RESPONSE_LENGTH   25
#define PART_0_LENGTH          1015
#define PART_1_LENGTH          911

// ---- Protocol: commands ---------------------------------------------------
#define SEARCH_RESPONSE 0x00
#define PING_RESPONSE   0x04
#define DUMP_RESPONSE   0x10
#define DIRECT_COMMAND  0x20

// ---- Protocol: byte offsets -----------------------------------------------
#define ID_BYTE          4
#define COMMAND_BYTE     6
#define PARAM_COUNT_BYTE 7
#define CHANNEL_BYTE     8   // first parameter entry: channel, param, hi, lo
#define PARAM_BYTE       9
#define VALUE_HI_BYTE    10
#define VALUE_LOW_BYTE   11
#define PARAM_ENTRY_SIZE 4
#define PART_BYTE        12

#define COMMAND_START 0xF0
#define TERMINATOR    0xF7

// ---- Parameter map dimensions ---------------------------------------------
#define SETUP_PARAMS    15  // params 2–11 and 20–24
#define INPUT_CHANNELS  4   // channels 1–4
#define INPUT_PARAMS    62  // params 2–63
#define OUTPUT_CHANNELS 6   // channels 5–10
#define OUTPUT_PARAMS   74  // params 2–75

// Where a parameter's value lives inside the cached dump. Values are 14 bit:
// the low 7 bits go into one byte, bit 7 is packed into a shared "middle" byte
// and the remaining high bits go into a third byte. byte == -1 means unused.
struct ByteLocation {
  int16_t part;
  int16_t byte;
};

struct BitLocation {
  int16_t part;
  int16_t byte;
  int16_t index;
};

struct DataLocation {
  ByteLocation low;
  BitLocation middle;
  ByteLocation high;
};

class Ultradrive {
 public:
  Ultradrive(HardwareSerial *serial, int rtsPin = 0, int ctsPin = 0);

  void enableFlowControl(bool enabled);
  void processIncoming(unsigned long now);
  void processOutgoing(Request *req);
  void writeDevice(Response *res);
  void writeDeviceStatus(Response *res);
  void writeDevices(Response *res);
  bool setSelected(int deviceId);
  int getSelected();

 private:
  struct Device {
    unsigned long lastResponse;
    byte response[SEARCH_RESPONSE_LENGTH];
  };

  size_t write(const uint8_t *buffer, size_t size);
  bool requestToSend(unsigned long timeout);
  void endSend();

  void search();
  void setTransmitMode(int deviceId);
  void ping(int deviceId);
  void dump(int deviceId, int part);

  void readCommands(unsigned long now);
  void handleFrame(unsigned long now);
  void applyCommands(const byte *frame, int length);
  void patchBuffer(int low, int high, const DataLocation &location);
  byte *partBuffer(int part);
  static const DataLocation *locate(int channel, int param);
  static bool isFresh(unsigned long timestamp, unsigned long now, unsigned long timeout);

  HardwareSerial *serial;
  int rtsPin;
  int ctsPin;
  bool flowControl;

  int selectedDevice;
  Device devices[MAX_DEVICES];

  // Cached state of the selected device
  byte dump0[PART_0_LENGTH];
  byte dump1[PART_1_LENGTH];
  byte pingResponse[PING_RESPONSE_LENGTH];
  bool hasDump0;
  bool hasDump1;
  bool invalidateSync;

  // Timers
  bool isFirstRun;
  unsigned long lastSearch;
  unsigned long lastPing;
  unsigned long lastResync;
  uint8_t unansweredPings;

  // Receive state
  bool readingCommand;
  int serialRead;
  byte serialBuffer[PART_0_LENGTH];
  byte serverBuffer[PART_0_LENGTH];

  static const byte vendorHeader[5];
  static const DataLocation setupLocations[SETUP_PARAMS];
  static const DataLocation inputLocations[INPUT_CHANNELS][INPUT_PARAMS];
  static const DataLocation outputLocations[OUTPUT_CHANNELS][OUTPUT_PARAMS];
};

#endif
