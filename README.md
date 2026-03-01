# ESPHomeLifx

Port of Lifx LAN UDP protocol to <https://ESPHome.io> firmware.

My #1 motivation in creating this was so I could host a giant light show using Light DJ (<https://lightdjapp.com/>).

LifxLAN protocol is __**fast**__ and processes packets in 1-2ms. Make your Tuya/esp8266 lights realtime-responsive with this protocol.

The other motivation for this project is Esphome integration with Home Assistant is somewhat clunky and requires configuring each device.  Bulbs running this protocol will all be detected automatically by the Lifx integration and do not need to be configured individually.

This firmware is otherwise stable (thanks Esphome!) and I have had 20+ bulbs running for multiple years without issue.

## !!! Warning

If you add this component it should be the last item in the YAML or it might cause the ESP to crash if wifi has not yet been initialized.

## Release Notes

### 0.6

- Added support for combined RGBWW lights (single light entity with RGB + cold white + warm white channels)
- Backward compatible: existing separate RGB + CWWW dual-light configurations still work
- Refactored as an ESPHome external component (no more `includes:` / `custom_component:` needed)
- Supports saving label/group/location/cloud values if set via the official app
- Added waveform effect support (SetWaveform / SetWaveformOptional packets): SAW, SINE, HALF_SINE, TRIANGLE, and PULSE waveforms
- Fixed res/ack flag handling

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
  - Cloud provisioning status/data is lost on bulb reboot at this time (fixed in 0.7)
- Fixed flaw in example yaml (platformio_options will break new compiles, now correct libraries directive)

### 0.3

- Responses now close enough to real bulbs that packet patterns are virtually identical
- Improved packet response accuracy

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

This is an ESPHome external component sourced directly from GitHub. Add it to your YAML and configure a light for LIFX emulation.

Two light configurations are supported:

### Option 1: Combined RGBWW light (recommended for RGBWW bulbs)

Use a single `rgbww` platform light with all 5 channels in one entity:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/giantorth/ESPHomeLifx
      ref: master
    components: [lifx_emulation]

time:
  - platform: sntp
    id: ha_time

light:
  - platform: rgbww
    id: rgbww_led
    name: "RGBWW Light"
    red: red_out
    green: green_out
    blue: blue_out
    cold_white: cold_white_out
    warm_white: warm_white_out
    cold_white_color_temperature: 6500 K
    warm_white_color_temperature: 2700 K

lifx_emulation:
  rgbww_led: rgbww_led
  time_id: ha_time
```

### Option 2: Separate RGB + CWWW lights (dual-light mode)

Use two separate light entities for color and white channels:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/giantorth/ESPHomeLifx
      ref: master
    components: [lifx_emulation]

time:
  - platform: sntp
    id: ha_time

light:
  - platform: rgb
    id: color_led
    name: "Color LED"
    red: red_out
    green: green_out
    blue: blue_out

  - platform: cwww
    id: white_led
    name: "White LED"
    cold_white: cold_white_out
    warm_white: warm_white_out
    cold_white_color_temperature: 6500 K
    warm_white_color_temperature: 2700 K

lifx_emulation:
  color_led: color_led
  white_led: white_led
  time_id: ha_time
```

### Configuration options

Full config not shown: *see `lifx_rgbww.yaml` and `lifx_dual.yaml` reference files in repo*

The component allows definition of the Lifx Location and Group values:

- `bulb_label` — defaults to the ESPHome device name if omitted
- `bulb_location` — location string, up to 32 characters (default: "ESPHome")
- `bulb_location_guid` — GUID for the location, randomly generate at <https://www.uuidgenerator.net/guid>
- `bulb_location_time` — epoch in msec * 1,000,000 (how real bulbs store this value) <https://currentmillis.com/>
- `bulb_group` — group string, up to 32 characters (default: "ESPHome")
- `bulb_group_guid` — GUID for the group
- `bulb_group_time` — epoch timestamp for the group

If a location/group label is different between bulbs for the same GUID the application uses the highest time as the authoritative source. Bulbs do not need to share this value and use current time when setting. Defaults are provided in code if not set.

It is highly suggested you lower the esphome logging for protocol performance:

```yaml
logger:
  level: WARN
  esp8266_store_log_strings_in_flash: False
```

## Supported Applications

- Official Lifx Windows/iOS/Android app control
  - Android app seems to work flawlessly
    - Android app has bug where it does not query device location/group values from bulb very often and uses cached values
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
- Supports combined RGBWW lights or separate RGB + CWWW dual-light setups
- Waveform effects (SAW, SINE, HALF_SINE, TRIANGLE, PULSE) with transient/non-transient and finite/infinite cycle support
## Lots of work still todo

- No real Lifx Cloud support (don't count on it either)
  - Required for Alexa/Google Home integration.  Use Home Assistant instead?
- Ignores packets coming from other offical bulbs yet (They seem to broadcast certain responses)
- Real bulb MAC addresses all start with D0:73:D5, haven't tried mirroring this to see if behavior changes
## Persistent State

The component saves bulb label, location, group, and cloud provisioning state to flash whenever they are changed at runtime (e.g. via the LIFX app). On boot, cloud state is always restored from flash. Label/location/group are restored only if the YAML defaults haven't changed (so updating YAML resets them to the new defaults).

To use this feature, you must enable `restore_from_flash` in your ESPHome platform config:

```yaml
esp8266:
  restore_from_flash: true
```

or for ESP32:

```yaml
esp32:
  restore_from_flash: true
```

Without this option, ESPHome preferences are not written to flash and saved state will not survive reboots.

## Debugging

- Enable debug logging with `debug: true` in the `lifx_emulation:` config block
- Debug output includes human-readable packet names (e.g. `LightSetColor`, `GetService`) alongside hex/decimal type codes
- Pull apart packets with Wireshark <https://github.com/mab5vot9us9a/WiresharkLIFXDissector>
- Official protocol documentation <https://lan.developer.lifx.com/docs/introduction>

Code used from <https://github.com/kayno/arduinolifx> and <https://github.com/area3001/esp8266_lifx>
