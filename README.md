# ESPHomeLifx
Port of Lifx protocol to ESPHome.io firmware.

!!! Warning !!!
If you add this component it should be the last item in the YAML or it might cause the ESP to crash.

Instructions:

- A sample bulb YAML is provided for ESPHome configuration
  - Light name will be same as Esphome name
  - Code expects you to have a 'white_led' and a 'color_led' device to control
- place lifx-emulation.h in your esphome folder (where all configs are located)

Working:

- Asynchronous UDP packet support for high-speed effects
- Can detect and control HSBK values from offical Lifx app
- LightDJ will detect and control light also
- Detected by lifxlan python library
- Windows Lifx software now believes this is an up-to-date Color 1000 bulb

Lots of work still todo:

- Home Assistant unable to detect bulbs, need to get wireshark involved
- Fake current firmware versions so app doesn't want to upgrade (Working on windows app now)
- iOS app flickers when checking bulb status
- Can't change bulb values with app - may use MQTT for configuration storage to avoid esp8266 flash limitations

Debugging:

- Serial console will output all in/out packet contents in HEX
- WILL CRASH A DEVICE WITHOUT A SERIAL PORT TO USE

Code used from https://github.com/area3001/esp8266_lifx
