# ESPHomeLifx

Port of Lifx LAN UDP protocol to <https://ESPHome.io> firmware.  

My #1 motivation in creating this was so I could host a giant light show using Light DJ (<https://lightdjapp.com/>).  

LifxLAN protocol is __**fast**__ and processes packets in 1-2ms. Make your Tuya/esp8266 lights realtime-responsive with this protocol.

The other motivation for this project is Esphome integration with Home Assistant is somewhat clunky and requires configuring each device.  Bulbs running this protocol will all be detected automatically by the Lifx integration and do not need to be configured individually.

This firmware is otherwise stable (thanks Esphome!) and I have had 20+ bulbs running for nearly a year without issue.

## !!! Warning

If you add this component it should be the last item in the YAML or it might cause the ESP to crash if wifi has not yet been initialized.

## Release Notes

### 0.5.1

- Change in packet response for MaxLifx support has broken Home Assistant integration, reverted to prior response behavior

### 0.5

- Now with colorZone packet support required for MaxLifx-z

### 0.4.1

- Fixed GUID ordering bug when app sets location or group and causes a duplicate area to appear

### 0.4

- Now understands cloud status/provisioning/config packets that come from app
- Bulbs now appear to the app as ready to join cloud, and when "joined" the iOS app caches info about bulbs much better (no more disappearing and reappearing!)
  - Bulbs do not talk to any cloud and only store provisioning data.  Buy real bulbs if you want a commercial cloud.
  - Cloud provisioning status/data is lost on bulb reboot at this time
- Fixed flaw in example yaml (platformio_options will break new compiles, now correct libraries directive)

### 0.3

- Responses now close enough to real bulbs that packet patterns are virtually identical
- Includes optional DiyHue entertainment socket support, must #define DIYHUE in .h file to enable

### 0.2.1

- Immediate wifi status reported in app and no longer 'goes dark' intermittently after viewing bulb info

### 0.2

- Still working on app stability issues with initial detection (more wireshark)
- Acknowledge all packets when requested properly
- Able to provide WiFi signal status to app
- Code cleanup, more packet types added for future support

### 0.1

- Custom component has changed and now supports setting Location/Group values
  - Use wireshark to sniff real values from other bulbs if you want to match their values
- Supports (runtime only) updates to Location/Group/Name
  - Resets to compile time config on reboot, MQTT is coming

## Instructions

- A sample bulb YAML is provided for ESPHome configuration
  - Light name will be same as Esphome name
  - Code expects you to have a 'white_led' and a 'color_led' device to control
  - Requires time component named 'ha_time'
- Place lifx-emulation.h in your esphome folder (where all configs are located)

Top of light config should have the correct includes and libraries.  
A time component is also required for this stack as shown below.  
It is highly suggested you lower the esphome logging for protocol performance.

```yaml
esphome:
  name: lifxtest
  platform: your_platform_here
  board: your_board_here
  # Required for LIFX LAN support
  includes:
    - lifx-emulation.h
  libraries:
    - ESPAsyncUDP

# Lifx emulation needs UTC time to respond to packets correctly.  
time:
  - platform: sntp
    id: ha_time

# Recommended esphome logger settings
logger:
  level: WARN
  esp8266_store_log_strings_in_flash: False
```

Full config not shown: *see reference yaml in repo*

The custom component allows definition of the Lifx Location and Group values.

- The Location/Group string can be up to 32 characters long
- GUIDs can be randomly generated if you are creating a new location/group for your bulbs <https://www.uuidgenerator.net/guid>
- The timestamp is epoch in msec * 1,000,000 (how real bulbs store this value) <https://currentmillis.com/>
  - If a location/group label is different between bulbs for the same GUID the application uses the highest time as the authoritative source
  - Bulbs do not need to share this value and use current time when setting
- Defaults for these values are in code if not set here

```yaml
# Required custom component for Lifx support
custom_component:
- lambda: |-
  auto LifxLAN = new lifxUdp();
  WiFi.macAddress(LifxLAN->mac);

  LifxLAN->set_bulbLabel( App.get_name().c_str() );  

  LifxLAN->set_bulbLocation( "Test Location" );
  LifxLAN->set_bulbLocationGUID( "4b833820-49b1-4f97-b324-316098f259d3" );
  LifxLAN->bulbLocationTime = 1600358586632000000;  // epoch in msec * 1,000,000

  LifxLAN->set_bulbGroup( "Test Group" );
  LifxLAN->set_bulbGroupGUID( "455820e8-3323-49f3-a7d0-598ba8092563" );
  LifxLAN->bulbGroupTime = 1600358586632000000; // epoch in msec * 1,000,000

  LifxLAN->beginUDP();
  return {LifxLAN};
```

## Supported Applications

- Official Lifx Windows/iOS/Android app control
  - Android app seems to work flawlessly
    - Android app has bug where it does not query device location/group values from bulb very often and uses cached values
  - ~~iOS only: Bulbs may not appear instantly when loading the mobile app~~
  - ~~iOS only: app seems to detect faster on first launch vs re-opening running app (warm start only remembers cloud bulbs??)~~
- LightDJ (<https://lightdjapp.com/>)
  - High speed music-reactive light shows for up to 128 lights (Philips Hue can only do 10 bulbs in entertainment mode!)
  - Party-tested with 35+ lights running for hours without issue
- HomeAssistant (<https://www.home-assistant.io/>)
  - Detected automatically with Lifx integration
- Logitech Harmony Remote
  - Automatically detects bulbs for use in activities (Turn on bias lighting with your tv)
- Compatability should be good enough for 3rd party applications

## Working/Implemented

- Firmware reports itself as an up-to-date Color 1000 model bulb
  - Running on <https://www.costco.com/feit-electric-wi-fi-smart-bulbs%2c-4-pack.product.100417461.html>
  - Flashed with Tuya-convert <https://github.com/ct-Open-Source/tuya-convert>
    - Warning newer firmware may not be OTA flashable (requires disassembly and soldering)
- Asynchronous UDP packet support for high-speed light show effects
  - ~20ms between changes with serial debugging on, ~2ms with serial debugging off
- Responses should mostly be identical to real bulb
  - Now with cloud provioning packet support (mobile app to bulb only)
- Appears in a Location/Group for supported applications
- Bulb can still integrate with DiyHue esphome configuration <https://github.com/diyhue/Lights/tree/master/ESPHome>
  - Optional DiyHue entertainment UDP socket support included in this code for multi-mode bulbs (#define DIYHUE)

## Lots of work still todo

- No real Lifx Cloud support (don't count on it either)
  - Required for Alexa/Google Home integration.  Use Home Assisistant or DiyHue instead?  
- Code is a single file, need to figure out how to include additional cpp/h files in esphome custom components to refactor
- Ignores packets coming from other offical bulbs yet (They seem to broadcast certain responses)
- Real bulb MAC addresses all start with D0:73:D5, haven't tried mirroring this to see if behavior changes
- Waveform bulb effects are not supported yet <https://lan.developer.lifx.com/docs/waveforms>
- Setting/Restoring state on boot not implemented yet
- No proper support for single RGB or RGBWW device control (yet)

## Debugging

- To enable, #define DEBUG in source file (off by default for performance)
  - Will still show packet rate in msec via serial when disabled
- Serial console will output all in/out packet contents in HEX
- Pull apart packets with Wireshark <https://github.com/mab5vot9us9a/WiresharkLIFXDissector>
- Official protocol documentation <https://lan.developer.lifx.com/docs/introduction>

Code used from <https://github.com/kayno/arduinolifx> and <https://github.com/area3001/esp8266_lifx>
