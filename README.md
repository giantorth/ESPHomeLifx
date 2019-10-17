# ESPHomeLifx
Port of Lifx protocol to ESPHome.io firmware.

Add to Esphome YAML.  Compile once before adding platformio_options.
```
esphome:
  name: lifxtest
  includes:
    - lifx-emulation.h
  platformio_options:
    lib_deps: ESPAsyncUDP
   
custom_component:
- lambda: |-
    auto lifxUdpSupport = new lifxUdp();
    return {lifxUdpSupport};

