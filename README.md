# ESPHomeLifx

Port of Lifx LAN <https://lan.developer.lifx.com/docs/introduction> UDP protocol to <https://ESPHome.io> firmware.  

My #1 motivation in creating this was so I could host a giant light show using Light DJ (<https://lightdjapp.com/>).  This app gives amazing light effects and supports up to 128 simultaneous bulbs using this protocol.  

The other motivation for this project is Esphome integration with Home Assistant is somewhat clunky and requires integration with each device.  Bulbs running this protocol will all be detected automatically by the Lifx integration and do not need to be configured indiviually.

This firmware is otherwise stable (thanks Esphome!) and I have had bulbs running for nearly a year without issue.

## !!! Warning

If you add this component it should be the last item in the YAML or it might cause the ESP to crash if wifi has not yet been initalized.

## Instructions

- A sample bulb YAML is provided for ESPHome configuration
  - Light name will be same as Esphome name
  - Code expects you to have a 'white_led' and a 'color_led' device to control
- Place lifx-emulation.h in your esphome folder (where all configs are located)

## Supported Applications

- Official Lifx Windows/iOS app control (Android not tested)
  - Bulbs may not appear instantly when loading the mobile app
- LightDJ (<https://lightdjapp.com/>)
  - High speed music-reactive light shows for up to 128 lights (Philips Hue can only do 10 bulbs in entertainment mode!)
  - Party-tested with 35 lights running for hours without issue
- HomeAssistant (<https://www.home-assistant.io/>)
  - Detected automatically with Lifx integration
- Compatability should be good enough for 3rd party applications

## Working/Implemented

- Firmware reports itself as an up-to-date Color 1000 model bulb
  - Running on <https://www.costco.com/feit-electric-wi-fi-smart-bulbs%2c-4-pack.product.100417461.html>
  - Flashed with Tuya-convert <https://github.com/ct-Open-Source/tuya-convert>
    - Warning newer firmware may not be OTA flashable (requires dissambly and soldering)
- Asynchronous UDP packet support for high-speed light show effects (~20ms between changes with debugging on, ~2ms with serial debugging off)
- Responses should mostly be identical to real bulb
- Appears in a (hardcoded) Location/Group for supported applications
- Bulb can still integrate with DiyHue esphome text sensor controls <https://github.com/diyhue/Lights/tree/master/ESPHome>

## Lots of work still todo

- Takes a some time to detect bulbs with iOS app and appear in group
  - I have over 20 custom bulbs (and another ~15 real bulbs) running this firmware, untested at lower counts to see if detection time is linear
  - Software seems to cache official bulbs better, even ones not connected to cloud
- No Lifx Cloud support
  - Required for Alexa/Google Home integration.  Use Home Assisistant or DiyHue instead?  
- Hardcoded Location/Group values ("My Home" and "Basement" respectively at this time)
  - Can't change bulb values with app - considering MQTT for configuration storage to avoid esp8266 flash limitations
- Code is a mess, need to figure out how to include additional cpp/h files in esphome custom components to refactor
- Doesn't understand packets coming from other offical bulbs yet (They seem to broadcast certain responses)
  - Currently not checking MAC target for correct hit or broadcast either
- Real bulb MAC addresses all start with D0:73:D5, haven't tried mirroring this to see if behavior changes
- Waveform bulb effects are not supported yet <https://lan.developer.lifx.com/docs/waveforms>
- Setting/Restoring state on boot not working properly yet

## Debugging

- Serial console will output all in/out packet contents in HEX
- Pull apart packets with Wireshark <https://github.com/mab5vot9us9a/WiresharkLIFXDissector>

Code used from <https://github.com/area3001/esp8266_lifx>
