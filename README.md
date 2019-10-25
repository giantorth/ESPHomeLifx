# ESPHomeLifx
Port of Lifx protocol to ESPHome.io firmware.

My #1 motivation in creating this was so I could host a giant light show using Light DJ (https://lightdjapp.com/).  This app gives amazing light effects and supports 128 simultaneous bulbs using the lifx lan protocol.  

!!! Warning !!!
If you add this component it should be the last item in the YAML or it might cause the ESP to crash.

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
- Pending code refactor
- Home Assistant unable to detect bulbs, need to get wireshark involved
- Fake current firmware versions so app doesn't want to upgrade
- App flickers when checking bulb status occasionally
- Somehow automatically read esphome name so it doesn't need to be set in lamba variable
- Packet sequence number not incrementing
- Does not support names/groups/locations yet 
- Does not support eeprom operations (not particularly recommended on an esp8266 anyways)

Debugging:
- Serial console will output all in/out packet contents in HEX

Code used from https://github.com/area3001/esp8266_lifx
