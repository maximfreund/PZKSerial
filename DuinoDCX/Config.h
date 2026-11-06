// =============================================================================
//  PilzkulturSound · Wi-Fi remote for the Behringer Ultradrive Pro DCX2496
//  Central configuration – everything you might want to tweak lives here.
// =============================================================================
#pragma once

// ---- Firmware ---------------------------------------------------------------
#define BRAND_NAME  "PilzkulturSound"
#define VERSION     "v0.1.0"
#define BUILD_DATE  __DATE__ " " __TIME__

// ---- Factory defaults -------------------------------------------------------
// Used until you change them on the web UI's settings page. A factory reset
// (see RESET_PIN) brings them back.
#define DEFAULT_SOFT_AP_SSID      "PilzkulturSound"
#define DEFAULT_SOFT_AP_PASSWORD  "nuklear1312"   // WPA2: 8–63 characters
#define DEFAULT_AUTH              "Basic RENYMjQ5NjpVbHRyYWRyaXZl"  // DCX2496:Ultradrive
#define DEFAULT_MDNS_NAME         "pilzkultur"   // → http://pilzkultur.local
#define DEFAULT_FLOW_CONTROL      false
#define DEFAULT_AUTO_DISABLE_AP   false

// ---- Pins -------------------------------------------------------------------
#define RX2_PIN    16  // RS-232 RX  ← TTL side of the MAX3232 / converter
#define TX2_PIN    17  // RS-232 TX  → TTL side of the MAX3232 / converter
#define RTS_PIN    21  // optional hardware flow control
#define CTS_PIN    22  // optional hardware flow control
#define RESET_PIN  13  // pull LOW while booting (> 1 s) for a factory reset

// ---- Serial -----------------------------------------------------------------
#define CONSOLE_BAUD          38400  // USB debug console
#define ULTRADRIVE_BAUD       38400  // fixed by the DCX2496
#define ULTRADRIVE_RX_BUFFER  4096   // holds a full state dump (~2 KB) with room to spare

// ---- Timing (ms) ------------------------------------------------------------
#define CONNECTION_TIMEOUT  10000  // wait this long for a Wi-Fi network to connect
#define RECONNECT_INTERVAL  20000  // auto-disable-AP mode: re-open the hotspot after
                                   // the Wi-Fi network has been gone this long
#define RESTART_DELAY        5000  // grace period before rebooting after an update
#define FACTORY_RESET_HOLD   1000  // how long RESET_PIN must be held LOW

// ---- Persistent settings (NVS) ----------------------------------------------
// Don't rename these: existing devices keep their settings under these keys,
// and the web UI uses the same names.
#define PREFERENCES_NAMESPACE  "duinodcx"
#define AUTH_KEY               "auth"
#define SOFT_AP_SSID_KEY       "apSsid"
#define SOFT_AP_PASSWORD_KEY   "apPassword"
#define MDNS_HOST_KEY          "mdnsHost"
#define FLOW_CONTROL_KEY       "flowControl"
#define AUTO_DISABLE_AP_KEY    "autoDisableAP"
#define POST_PARAM_SSID_KEY    "ssid"
#define POST_PARAM_PASSWORD_KEY "password"

// ---- Buffer sizes (incl. NUL terminator) ------------------------------------
#define AUTH_LENGTH              200  // "Basic <base64(user:password)>"
#define SSID_LENGTH               33  // 802.11 limit: 32 characters
#define PASSWORD_LENGTH           65  // WPA2 limit: 64 characters
#define MDNS_NAME_LENGTH          65
#define FORM_NAME_LENGTH          16
#define MIN_WPA2_PASSWORD_LENGTH   8
