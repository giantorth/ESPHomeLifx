#pragma once

#include "esphome/core/component.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/time/real_time_clock.h"
#ifdef USE_ESP8266
#include <ESP8266WiFi.h>
#elif defined(USE_ESP32)
#include <WiFi.h>
#endif
#include <ESPAsyncUDP.h>

#include "lifx_protocol.h"

namespace esphome {
namespace lifx_emulation {

class LifxEmulation : public Component
{
public:
	// ---- Setters called by generated code (from __init__.py to_code()) ----
	void set_color_led(light::LightState *light) { this->color_led_ = light; }
	void set_white_led(light::LightState *light) { this->white_led_ = light; }
	void set_rgbww_led(light::LightState *light) { this->rgbww_led_ = light; }
	void set_time(time::RealTimeClock *time_rtc) { this->ha_time_ = time_rtc; }

	void set_debug(bool debug) { this->debug_ = debug; }

	void set_bulb_label(const char *arg) { strncpy(bulbLabel, arg, sizeof(bulbLabel) - 1); }

	void set_bulb_location(const char *arg) { strncpy(bulbLocation, arg, sizeof(bulbLocation) - 1); }
	void set_bulb_location_guid(const char *arg) { strncpy(bulbLocationGUID, arg, sizeof(bulbLocationGUID) - 1); bulbLocationGUID[sizeof(bulbLocationGUID) - 1] = '\0'; }
	void set_bulb_location_time(uint64_t arg) { bulbLocationTime = arg; }

	void set_bulb_group(const char *arg) { strncpy(bulbGroup, arg, sizeof(bulbGroup) - 1); }
	void set_bulb_group_guid(const char *arg) { strncpy(bulbGroupGUID, arg, sizeof(bulbGroupGUID) - 1); bulbGroupGUID[sizeof(bulbGroupGUID) - 1] = '\0'; }
	void set_bulb_group_time(uint64_t arg) { bulbGroupTime = arg; }

	// ---- ESPHome Component lifecycle ----
	void setup() override;
	void loop() override {}

	// Run after WiFi is established so the UDP listener can bind successfully
	float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

	byte mac[6] = {};

	char bulbLabel[32] = "";
	char bulbLocation[32] = "ESPHome";
	char bulbLocationGUID[37] = "b49bed4d-77b0-05a3-9ec3-be93d9582f1f";
	uint64_t bulbLocationTime = 1553350342028441856;

	char bulbGroup[32] = "ESPHome";
	char bulbGroupGUID[37] = "bd93e53d-2014-496f-8cfd-b8886f766d7a";
	uint64_t bulbGroupTime = 1600213602318000000;

	uint8_t cloudStatus = 0x00;
	byte cloudBrokerUrl[33] = {0x00}; // Unclouded response
	unsigned char cloudAuthResponse[32] = {0x00}; // Unclouded response
	byte authResponse[56] = {0x00};

private:
	// ---- Member pointers set via setters ----
	light::LightState *color_led_{nullptr};
	light::LightState *white_led_{nullptr};
	light::LightState *rgbww_led_{nullptr};
	time::RealTimeClock *ha_time_{nullptr};
	bool debug_{false};

	bool is_combined_mode() { return this->rgbww_led_ != nullptr; }

	float maxColor = 255;
	// initial bulb values - warm white!
	uint16_t power_status = 65535;
	uint16_t hue = 0;
	uint16_t sat = 0;
	uint16_t bri = 65535;
	uint16_t kel = 2700;
	uint8_t trans = 0;
	uint32_t period = 0;
	float cycles = 0;
	int skew_ratio = 0; //signed 16 bit int
	uint8_t waveform = 0;
	uint8_t set_hue = 1;
	uint8_t set_sat = 1;
	uint8_t set_bri = 1;
	uint8_t set_kel = 1;
	long dim = 0;
	uint32_t dur = 0;
	uint8_t _sequence = 0;
	unsigned long lastChange = millis();
	uint32_t tx_bytes = 0;
	uint32_t rx_bytes = 0;

	byte site_mac[6] = {0x4C, 0x49, 0x46, 0x58, 0x56, 0x32}; // spells out "LIFXV2"

	// tags for this bulb, seemingly unused on current real bulbs
	char bulbTags[LifxBulbTagsLength] = {
		0, 0, 0, 0, 0, 0, 0, 0};
	char bulbTagLabels[LifxBulbTagLabelsLength] = "";

	// real guids are stored as byte arrays instead of a char string representation
	byte bulbGroupGUIDb[16] = {};
	byte bulbLocationGUIDb[16] = {};
	// Guids in packets come in a bizarre mix of big and little endian
	uint8_t guidSeq[16] = {3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};

	AsyncUDP Udp;

	// ---- Method declarations (implemented in lifx_emulation.cpp) ----
	void beginUDP();
	void incomingUDP(AsyncUDPPacket &packet);
	void processRequest(byte *packetBuffer, float packetSize, LifxPacket &request);
	void handleRequest(LifxPacket &request, AsyncUDPPacket &packet);
	unsigned int sendPacket(LifxPacket &pkt, AsyncUDPPacket &Udpi);
	void setLight();
	void setLightCombined();
	void setLightDual();
};

} // namespace lifx_emulation
} // namespace esphome
