# Touchy-pad

## Current features

* A "premium feel" open-source multitouch USB touchpad with built in customizable screen (for use with Mac/Linux/Windows/Android).  Even if you don't want to run our sister app.
* Works with cheap ESP32 based display boards ($15-$30USD depending on features) - no soldering required, just connect USB, run the installer and go.
* When used as a touchpad providea a cute water droplet touch/grab/turn/gesture visualization
* Customizable screen layouts (define with JSON or a python API), bind controls to built-in keyboard/mouse macros or host side python behaviors.  A full set of widgets are available (based on [LVGL](https://lvgl.io/)).
* Allows simple key/mouse shortcut/macros without need for leaving the stream-controller app running.  Generated entirely natively as USB events from our device.
* Automated installer provides 'one-click' install for boards you purchased from wherever.

## Features coming soon

* Provides a [Stream-controller](https://streamcontroller.github.io/docs/latest/) compatible API so that a graphical button array can be selected instead of touchpad.  
* Support for more devices (including little tiny 3 or 4" displays - suitable for building into custom keyboards or PC cases)

## Eventual features

* Stylus recognition (on suitable device), for brush effects etc...
* Multitouch is currently supported entirely in the device (by emulating appropriate USB HID actions), but for some art applications we should also expose a multitouch HID endpoint
* A lasercut or 3d printed template to allow those critical buttons to have physical 'feel' separating them from the touchpad/other buttons.
* If haptic hardware installed haptically render taps/clicks/buttons feels. (Leaves screen mechanically isolated from case (for better haptics))
* Add optional mouse left/right/middle click buttons anywhere in the screen layout
* Add an expand/shrink touchpad hotkey (possibly by using existing screen abstraction)

## Supported hardware

There are [lots](docs/hardware.md) of $15 USD ish "Cheap Yellow Displays" that work with this project.  See below for how to run the installer (it will flash the firmware onto your device).  After installing, just connect the device to USB and you should be good to go.

## Documentation

FIXME add install guide
* Current [design documents](docs/README.md).
* [Developer setup](docs/development.md) — new-machine setup, `just` recipes, git hooks.
* Current rough [TODO list](TODO.md).

## Doc links

* [platform io config](https://community.home-assistant.io/t/esp32-8048s050-sunton-5-0-cyd-config/782740) - seems to use gt911 touch controller which seems quite nice
* [general info](https://www.espboards.dev/esp32/cyd-esp32-8048s050/) on these boards
* [great platform io repo](https://github.com/rzeldent/platformio-espressif32-sunton)

## Development and AI slop

## Credits/Thanks

## License
Copyright © 2026 Kevin Hester.
Licensed under the [GNU General Public License v3.0 or later](LICENSE).
