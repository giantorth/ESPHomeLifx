# ESPHomeLifx
Port of Lifx protocol to ESPHome.io firmware.

My #1 motivation in creating this was so I could host a giant light show using Light DJ (https://lightdjapp.com/).  This app gives amazing light effects and supports 128 simultaneous bulbs using the lifx lan protocol.  

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
- Lifx software now believes this is an up-to-date Color 1000 bulb
- Home Assistant can detect and control bulbs

Lots of work still todo:

- Takes a long time to detect with iOS app and appear in correct group
- Hardcoded Location/Group values ("My Home" and "Basement" respectively at this time)
- iOS app flickers when checking bulb status
- Can't change bulb values with app - may use MQTT for configuration storage to avoid esp8266 flash limitations


Debugging:

- Serial console will output all in/out packet contents in HEX
- WILL CRASH A DEVICE WITHOUT A SERIAL PORT TO USE

Code used from https://github.com/area3001/esp8266_lifx
