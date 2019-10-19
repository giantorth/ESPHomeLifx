# ESPHomeLifx
Port of Lifx protocol to ESPHome.io firmware.

Add to Esphome YAML.  
```
esphome:
  name: lifxtest
  includes:
    - lifx-emulation.h
  libraries:
    lib_deps: ESPAsyncUDP
   
custom_component:
- lambda: |-
    auto lifxUdpSupport = new lifxUdp();
    return {lifxUdpSupport};

