# ESPHomeLifx
Port of Lifx protocol to ESPHome.io firmware.

!!! Warning !!!
There is a bug where this will crash immidately if it is unable to write to serial. Flash this to sealed devices at your own risk!!

Instructions:
- A sample bulb YAML is provided for ESPHome configuration
  - Update light name in custom component lambda at bottom of config
  - Code expects you to have a 'white_led' and a 'color_led' device to control
- place lifx-emulation.h in your esphome folder (where all configs are located)

Working:
- Can detect and control HSBK values from offical Lifx app
- LightDJ will detect and control light also
- Detected by lifxlan python library

Lots of work still todo:
- Home Assistant unable to detect bulbs, need to get wireshark involved
- Fake current firmware versions so app doesn't want to upgrade
- app flickers when checking bulb status occasionally
- Somehow automatically read esphome name so it doesn't need to be set in lamba variable
- Packet sequence number not incrementing

Debugging:
- Serial console will output all in/out packet contents in HEX

Code used from https://github.com/area3001/esp8266_lifx
