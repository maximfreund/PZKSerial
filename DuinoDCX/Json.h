// =============================================================================
//  Tiny streaming JSON writer – just enough for the web API's flat replies.
//  Strings are escaped properly, so SSIDs or passwords containing quotes or
//  backslashes can't break the UI.
//
//    {
//      JsonObject json(res);
//      json.add("version", VERSION).add("buildDate", BUILD_DATE);
//    }  // closing brace is written when `json` goes out of scope
// =============================================================================
#pragma once

#include <Print.h>

inline void printJsonString(Print &out, const char *value) {
  out.print('"');

  for (const char *c = value; *c; c++) {
    switch (*c) {
      case '"':
        out.print("\\\"");
        break;
      case '\\':
        out.print("\\\\");
        break;
      case '\n':
        out.print("\\n");
        break;
      case '\r':
        out.print("\\r");
        break;
      case '\t':
        out.print("\\t");
        break;
      default:
        if ((unsigned char)*c < 0x20) {
          out.printf("\\u%04x", (unsigned char)*c);
        } else {
          out.print(*c);
        }
    }
  }

  out.print('"');
}

class JsonObject {
 public:
  explicit JsonObject(Print &out) : out(out), empty(true) { out.print('{'); }
  ~JsonObject() { out.print('}'); }

  JsonObject &add(const char *key, const char *value) {
    if (!empty) {
      out.print(',');
    }
    empty = false;
    printJsonString(out, key);
    out.print(':');
    printJsonString(out, value);
    return *this;
  }

 private:
  Print &out;
  bool empty;
};

class JsonArray {
 public:
  explicit JsonArray(Print &out) : out(out), empty(true) { out.print('['); }
  ~JsonArray() { out.print(']'); }

  JsonArray &add(const char *value) {
    if (!empty) {
      out.print(',');
    }
    empty = false;
    printJsonString(out, value);
    return *this;
  }

 private:
  Print &out;
  bool empty;
};
