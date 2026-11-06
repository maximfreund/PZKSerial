# PilzkulturSound

[![Build firmware](https://github.com/maximfreund/PZKSerial/actions/workflows/build.yml/badge.svg)](https://github.com/maximfreund/PZKSerial/actions/workflows/build.yml)

**Wi-Fi remote control for the Behringer Ultradrive Pro DCX2496.** An ESP32 hooks up to the DCX's RS-232 port, opens its own hotspot and serves a full web UI to any phone, tablet or laptop. No app, no cables across the venue.

Decoupled fork of [DuinoDCX](https://github.com/lasselukkari/DuinoDCX) by Lasse Lukkari.

<img src="https://i.imgur.com/2BB5tPi.png" width="100%" alt="Web UI" />

## Quick start

1. Power up the ESP32 that is wired to the DCX2496.
2. Join the Wi-Fi **`PilzkulturSound`**. The password is `Ultradrive`.
3. Open **http://192.168.4.1**, or http://ultradrive.local.
4. Log in as `DCX2496` with the password `Ultradrive`.
5. **Change the hotspot password and the login under Settings.** The defaults are public.

> Already flashed an older build? Units that have saved settings before keep their old hotspot name. Change it under Settings or do a [factory reset](#factory-reset).

Try the UI without hardware: [online demo](https://lasselukkari.github.io/dcx-ui) · [demo video](https://streamable.com/z9yxto) (thanks to *DCX Link*).

## Hardware

* ESP32 dev board
* RS-232 ↔ TTL converter (MAX3232, runs on 3.3 V), or the [SerialChiller](https://github.com/lasselukkari/SerialChiller) board

| ESP32 pin | Goes to |
| --- | --- |
| GPIO 16 (RX) | converter TTL output: data **from** the DCX |
| GPIO 17 (TX) | converter TTL input: data **to** the DCX |
| GPIO 21 / 22 | RTS / CTS, optional, only if you enable flow control |
| GPIO 13 | factory reset button to GND, optional |
| 3V3, GND | converter power |

Not sure about the wiring? Flash [HardwareTest](HardwareTest/HardwareTest.ino) first: it is an RS-232 loopback test that also checks polarity. The upstream [user guide](https://lasselukkari.github.io/DuinoDCX) walks through the whole hardware build and the DCX setup.

## Features

* **Hotspot** that is always on (the default), or switches off automatically once the unit has joined a Wi-Fi network. If that network disappears for 20 s, the hotspot comes back on its own, so you can't lock yourself out.
* **Join an existing Wi-Fi** from the Settings page.
* **Firmware updates over the air**: drag a `.bin` onto the Settings page.
* **Live meters and full parameter control**, kept in sync when someone turns knobs on the unit itself.

## Factory reset

Hold GPIO 13 to GND while powering on, for more than 1 s. This restores the hotspot name and password, the login and the mDNS name. A saved Wi-Fi network is kept.

## Building

You need Node.js 18 or newer, plus Arduino IDE 2 or `arduino-cli` with the **ESP32 core 3.x** (tested with 3.3.7).

```sh
npm install
npm run build    # brands the web UI and generates DuinoDCX/StaticFiles.h
```

Then open `DuinoDCX/DuinoDCX.ino` in the Arduino IDE. Pick **ESP32 Dev Module**, set **Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA)** and upload. With `arduino-cli` on your PATH, `npm run compile` does the same build.

What to change where:

| What | Where |
| --- | --- |
| Default hotspot, login, mDNS name, pins, timings | [`DuinoDCX/Config.h`](DuinoDCX/Config.h) |
| Name shown in the browser tab, repo checked for updates | `brand` in [`package.json`](package.json) |

### Releases

Every push to `master` builds the firmware on GitHub Actions; download it from the run's *Artifacts*. To publish a release, bump `VERSION` in `Config.h` and push a matching tag, for example `v0.1.0`. CI then attaches `duinodcx-v0.1.0.bin` to the GitHub release. The UI's update page offers the newest release to any unit that runs a different version, as long as the browser has internet access.

## Project layout

```
DuinoDCX/
  DuinoDCX.ino       Wi-Fi, web API, setup and loop
  Config.h           every default and pin in one place
  Ultradrive.*       DCX2496 RS-232 protocol and state cache
  UltradriveMap.cpp  where each parameter lives inside the state dump
  Json.h, aWOT.*     web server bits
scripts/brand-ui.js  applies the PilzkulturSound branding to the UI bundle
HardwareTest/        RS-232 loopback test
```

The web UI itself is the separate [dcx-ui](https://github.com/lasselukkari/dcx-ui) project, pulled in through npm.

## Known limitations

* No preset saving or recalling.
* Linked devices (several DCX units on one RS-232 chain) don't work.
* The web UI is plain HTTP with basic auth. Anyone who joins the hotspot and knows the login can change your system and flash firmware, so change the defaults.

## Credits

[DuinoDCX](https://github.com/lasselukkari/DuinoDCX) and [dcx-ui](https://github.com/lasselukkari/dcx-ui) by Lasse Lukkari. Ilkka Huhtakallio contributed the transfer function code behind the graphs.

## License & disclaimer

[MIT](LICENSE). Use at your own risk: nobody takes responsibility if this damages your sound system.
