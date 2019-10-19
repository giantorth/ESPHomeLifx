# ESPHomeLifx
Port of Lifx protocol to ESPHome.io firmware.

Instructions:
- A sample bulb YAML is provided for ESPHome configuration
  - Update light name in custom component lambda at bottom of config
- place lifx-emulation.h in your esphome folder (where all configs are located)
- ESPHome light component must be named "lifx"

Working:
- Can detect and control HSBK values from offical Lifx app
- LightDJ will detect and control light also
- Detected by lifxlan python library

Lots of work still todo:
- Fake current firmware versions so app doesn't want to upgrade
- app flickers when checking bulb status
- Somehow automatically read esphome name so it doesn't need to be set in lamba variable
- Code written for RGB led only
- Sequence number not incrementing

Debugging:
- Serial console will output all in/out packet contents in HEX

Code used from https://github.com/area3001/esp8266_lifx
