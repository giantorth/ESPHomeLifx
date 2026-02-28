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

// Uncomment to enable verbose serial debug output
//#define LIFX_DEBUG

#ifdef LIFX_DEBUG
#define debug_print(x, ...) Serial.print(x, ##__VA_ARGS__)
#define debug_println(x, ...) Serial.println(x, ##__VA_ARGS__)
#else
#define debug_print(x, ...)
#define debug_println(x, ...)
#endif

static const char *const TAG = "lifx_emulation";

// this structure doesn't support the response/ack byte properly and needs updated.
struct LifxPacket
{
	////frame
	uint16_t size;	   //little endian
	uint16_t protocol; //little endian
	byte source[4];	   //source

	////frame address
	byte bulbAddress[6]; //device mac
	uint16_t reserved2;	 // mac padding
	byte site[6];		 // "reserved"

	uint8_t res_ack;
	uint8_t sequence; //message sequence number

	////protocol header
	uint64_t timestamp;
	uint16_t packet_type; //little endian
	uint16_t reserved4;

	byte data[128];
	int data_size;
};

//need to verify these
const unsigned int LifxProtocol_AllBulbsResponse = 21504; // 0x5400
const unsigned int LifxProtocol_AllBulbsRequest = 13312;  // 0x3400
const unsigned int LifxProtocol_BulbCommand = 5120;		  // 0x1400

const unsigned int LifxPacketSize = 36;
const unsigned int LifxPort = 56700; // local port to listen on
const unsigned int LifxBulbLabelLength = 32;
const unsigned int LifxBulbTagsLength = 8;
const unsigned int LifxBulbTagLabelsLength = 32;
#define LIFX_MAX_PACKET_LENGTH 512

// firmware versions, etc
const byte LifxBulbVendor = 1;
const byte LifxBulbProduct = 22;
const byte LifxBulbVersion = 0;
const byte LifxFirmwareVersionMajor = 1;
const byte LifxFirmwareVersionMinor = 5;
const unsigned int LifxMagicNum = 614500; // still a mystery

const byte SERVICE_UDP = 0x01;
const byte SERVICE_TCP = 0x02;
const byte SERVICE_UDP5 = 0x05; // Real bulbs seem to ofer this service too...

// Packet RES/ACK values
const byte NO_RESPONSE = 0x00;
const byte RES_REQUIRED = 0x01;
const byte ACK_REQUIRED = 0x02;
const byte RES_ACK_REQUIRED = 0x03;		// unknown value, no real packets seem to set both
const byte PAN_REQUIRED = 0x04;			// Set on GET_PAN_GATEWAY (third bit on only)
const byte RES_PAN_REQUIRED = 0x05;		// set on multiple packets (first and third bit set)
const byte ACK_PAN_REQUIRED = 0x06;		// bits 2/3 on
const byte RES_ACK_PAN_REQUIRED = 0x07; // All three bits set

// Device Messages
const byte GET_PAN_GATEWAY = 0x02;
const byte PAN_GATEWAY = 0x03; // stateService(3)

const byte GET_HOST_INFO = 0x0c; // getHostInfo(12)
const byte HOST_INFO = 0x0d;	 // stateHostInfo(13)

const byte GET_MESH_FIRMWARE_STATE = 0x0e; //getHostFirmware(14)
const byte MESH_FIRMWARE_STATE = 0x0f;	   // stateHostFirmware(15)

const byte GET_WIFI_INFO = 0x10; // getWifiInfo (16)
const byte WIFI_INFO = 0x11;	 // stateWifiInfo (17)

const byte GET_WIFI_FIRMWARE_STATE = 0x12; // getWifiFirmware(18)
const byte WIFI_FIRMWARE_STATE = 0x13;	   // stateWifiFirmware(19)

const byte GET_POWER_STATE = 0x14; // getPower(20)
const byte SET_POWER_STATE = 0x15; // setPower(21)
const byte POWER_STATE = 0x16;	   // statePower(22)

const byte GET_BULB_LABEL = 0x17; // getLabel(23)
const byte SET_BULB_LABEL = 0x18; // setLabel(24)
const byte BULB_LABEL = 0x19;	  // stateLabel(25)

const byte GET_BULB_TAGS = 0x1a; // unlisted packets
const byte SET_BULB_TAGS = 0x1b; //
const byte BULB_TAGS = 0x1c;	 //

const byte GET_BULB_TAG_LABELS = 0x1d; // unlisted packets
const byte SET_BULB_TAG_LABELS = 0x1e;
const byte BULB_TAG_LABELS = 0x1f;

const byte GET_VERSION_STATE = 0x20; // getVersion(32)
const byte VERSION_STATE = 0x21;	 // stateVersion(33)

const byte GET_INFO = 0x22;	  // getInfo(34)
const byte STATE_INFO = 0x23; // stateInfo(35)

const byte RESET_BULB_ANDROID = 0x37; // Sent serval times on reset from android

// no documented packets 0x24 thru 0x2c

const byte ACKNOWLEDGEMENT = 0x2d; // acknowledgement(45)
const byte RESET_BULB = 0x2e;	   // sent on bulb reset?

const byte GET_LOCATION_STATE = 0x30; // getLocation(48)
const byte SET_LOCATION_STATE = 0x31; // setLocation(49)
const byte LOCATION_STATE = 0x32;	  // stateLocation(50)

const byte GET_GROUP_STATE = 0x33; // getGroup(51)
const byte SET_GROUP_STATE = 0x34; // setGroup(52)
const byte GROUP_STATE = 0x35;	   // stateGroup(53) - res_required

// suspect these are FW checksum values
const byte GET_AUTH_STATE = 0x36; // getAuth(54)
const byte SET_AUTH_STATE = 0x37; // setAuth(55)
const byte AUTH_STATE = 0x38;	  // stateAuth(56) - should always respond from a SET

const byte ECHO_REQUEST = 0x3a;	 // echoRequest(58)
const byte ECHO_RESPONSE = 0x3b; // echoResponse(59)

// Light Messages
const byte GET_LIGHT_STATE = 0x65; // get(101)
const byte SET_LIGHT_STATE = 0x66; // setColor(102)
const byte SET_WAVEFORM = 0x67;	   // setWaveform(103)

const byte LIGHT_STATUS = 0x6b; // state(107) - res_required

const byte GET_POWER_STATE2 = 0x74; // getPower(116)
const byte SET_POWER_STATE2 = 0x75; // setPower(117)
const byte POWER_STATE2 = 0x76;		// statePower(118)

const byte SET_WAVEFORM_OPTIONAL = 0x77; // setWaveformOptional(119)

const byte GET_INFARED_STATE = 0x78;   // getInfared(120)
const byte STATE_INFARED_STATE = 0x79; // stateInfared(121)
const byte SET_INFARED_STATE = 0x7A;   // setInfrared(122)

// Get/State happens during bulb info screens, Set happens during reset
const byte GET_CLOUD_STATE = 0xc9; // getCloud(201)
const byte SET_CLOUD_STATE = 0xca; // setCloudState (202)
const byte CLOUD_STATE = 0xcb;	   // stateCloud(203)

const byte GET_CLOUD_AUTH = 0xcc;	// (204)
const byte SET_CLOUD_AUTH = 0xcd;	// (205)
const byte CLOUD_AUTH_STATE = 0xce; // (206)

const byte GET_CLOUD_BROKER = 0xd1;	  // (209)
const byte SET_CLOUD_BROKER = 0xd2;	  // (210)
const byte CLOUD_BROKER_STATE = 0xd3; // (211)

const uint16_t GET_COLOR_ZONE = 502; // getColorZones(502)
const uint16_t STATE_COLOR_ZONE = 503; // stateColorZone(503)

#define SPACE " "

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
	void setup() override
	{
		// Read MAC address from WiFi hardware
		WiFi.macAddress(this->mac);
		this->beginUDP();
	}

	void loop() override
	{
	}

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

	/******************************************************************************************************************
	 * beginUDP()
	 * Creates the UDP listener. Called from setup().
	******************************************************************************************************************/
	void beginUDP()
	{
		debug_print(F("Setting Light Name: "));
		debug_println(bulbLabel);
		if (debug_) ESP_LOGD(TAG, "Setting Light Name: %s", bulbLabel);

		// Convert incoming guid strings to byte arrays (strips dashes)
		hexCharacterStringToBytes(bulbGroupGUIDb, (const char *)bulbGroupGUID);
		hexCharacterStringToBytes(bulbLocationGUIDb, (const char *)bulbLocationGUID);

		debug_print("Wifi Signal: ");
		debug_println(WiFi.RSSI(), DEC);
		if (debug_) ESP_LOGD(TAG, "Wifi Signal: %d", WiFi.RSSI());

		// start listening for packets
		if (Udp.listen(LifxPort))
		{
			ESP_LOGW("LIFXUDP", "Lifx Emulation UDP listener Enabled");
			Udp.onPacket(
				[&](AsyncUDPPacket &packet) {
					unsigned long packetTime = millis();
					uint32_t packetSize = packet.length();
					if (packetSize)
					{ //ignore empty packets
						incomingUDP(packet);
					}
					if (debug_) ESP_LOGD(TAG, "Response: %lu msec", millis() - packetTime);
				});
		}
		//TODO: TCP support necessary?

		setLight(); // sync initial light state
	}

	/******************************************************************************************************************
	 * incomingUDP( AsyncUDPPacket )
	 * Called on all incoming UDP packets
	******************************************************************************************************************/
	void incomingUDP(AsyncUDPPacket &packet)
	{
		int packetSize = packet.length();
		rx_bytes += packetSize;

		uint8_t packetBuffer[packetSize];
		memcpy(packetBuffer, packet.data(), packetSize);

		debug_println();
		debug_print(F("Packet Arrived ("));
		IPAddress remote_addr = (packet.remoteIP());
		IPAddress local_addr = packet.localIP();
		int remote_port = packet.remotePort();
		if (debug_) ESP_LOGD(TAG, "Packet Arrived (%s:%d->%s)(%d bytes)",
			remote_addr.toString().c_str(), remote_port,
			local_addr.toString().c_str(), packetSize);
		debug_print(remote_addr);
		debug_print(F(":"));
		debug_print(remote_port);
		debug_print("->");
		debug_print(local_addr);
		debug_print(F(")("));
		debug_print(packetSize);
		debug_print(F(" bytes): "));
		for (int i = 0; i < packetSize; i++)
		{
			if (packetBuffer[i] <= 0x0F)
			{
				debug_print(F("0"));
			}
			debug_print(packetBuffer[i], HEX);
			debug_print(SPACE);
		}
		debug_println();

		LifxPacket request;
		processRequest(packetBuffer, packetSize, request);
		handleRequest(request, packet);
	}

	/******************************************************************************************************************
	 * processRequest( byte, float, LifxPacket )
	 * Unpacks a raw UDP buffer into a LifxPacket struct
	******************************************************************************************************************/
	void processRequest(byte *packetBuffer, float packetSize, LifxPacket &request)
	{
		request.size = packetBuffer[0] + (packetBuffer[1] << 8);	 //little endian
		request.protocol = packetBuffer[2] + (packetBuffer[3] << 8); //little endian

		byte sourceID[] = {packetBuffer[4], packetBuffer[5], packetBuffer[6], packetBuffer[7]};
		memcpy(request.source, sourceID, 4);

		byte bulbAddress[] = {
			packetBuffer[8], packetBuffer[9], packetBuffer[10], packetBuffer[11], packetBuffer[12], packetBuffer[13]};
		memcpy(request.bulbAddress, bulbAddress, 6);

		request.reserved2 = packetBuffer[14] + packetBuffer[15];

		byte site[] = {
			packetBuffer[16], packetBuffer[17], packetBuffer[18], packetBuffer[19], packetBuffer[20], packetBuffer[21]};
		memcpy(request.site, site, 6);

		request.res_ack = packetBuffer[22];
		request.sequence = packetBuffer[23];
		request.timestamp = packetBuffer[24] + packetBuffer[25] + packetBuffer[26] + packetBuffer[27] +
							packetBuffer[28] + packetBuffer[29] + packetBuffer[30] + packetBuffer[31];
		request.packet_type = packetBuffer[32] + (packetBuffer[33] << 8); //little endian
		request.reserved4 = packetBuffer[34] + packetBuffer[35];

		int j = 0;
		for (unsigned int i = LifxPacketSize; i < packetSize; i++)
		{
			request.data[j++] = packetBuffer[i];
		}

		request.data_size = j;
	}

	void handleRequest(LifxPacket &request, AsyncUDPPacket &packet)
	{
		debug_print(F("-> Packet 0x"));
		debug_print(request.packet_type, HEX);
		debug_print(F(" ("));
		debug_print(request.packet_type, DEC);
		debug_println(F(")"));
		if (debug_) ESP_LOGD(TAG, "-> Packet 0x%02X (%d)", request.packet_type, request.packet_type);

		LifxPacket response;
		for (int x = 0; x < 4; x++)
		{
			response.source[x] = request.source[x];
		}
		// Bulbs must respond with matching sequence number from request
		response.sequence = request.sequence;

		response.res_ack = NO_RESPONSE;

		switch (request.packet_type)
		{
		case GET_PAN_GATEWAY:
		{
			response.packet_type = PAN_GATEWAY;
			response.protocol = LifxProtocol_AllBulbsResponse;
			byte UDPdata[] = {
				SERVICE_UDP,
				lowByte(LifxPort),
				highByte(LifxPort),
				0x00,
				0x00};
			byte UDPdata5[] = {
				SERVICE_UDP5,
				lowByte(LifxPort),
				highByte(LifxPort),
				0x00,
				0x00};

			// A real bulb responds twice, once as service type 5
			memcpy(response.data, UDPdata, sizeof(UDPdata));
			response.data_size = sizeof(UDPdata);
			sendPacket(response, packet);
			memcpy(response.data, UDPdata5, sizeof(UDPdata));
			response.data_size = sizeof(UDPdata);
			sendPacket(response, packet);
		}
		break;

		case SET_LIGHT_STATE:
		{
			hue = word(request.data[2], request.data[1]);
			sat = word(request.data[4], request.data[3]);
			bri = word(request.data[6], request.data[5]);
			kel = word(request.data[8], request.data[7]);
			dur = (uint32_t)request.data[9] << 0 |
				  (uint32_t)request.data[10] << 8 |
				  (uint32_t)request.data[11] << 16 |
				  (uint32_t)request.data[12] << 24;

			setLight();
			if (request.res_ack == RES_REQUIRED)
			{
				response.res_ack = NO_RESPONSE;
				response.packet_type = LIGHT_STATUS;
				response.protocol = LifxProtocol_AllBulbsResponse;
				byte StateData[52] = {
					lowByte(hue),
					highByte(hue),
					lowByte(sat),
					highByte(sat),
					lowByte(bri),
					highByte(bri),
					lowByte(kel),
					highByte(kel),
					lowByte(dim),
					highByte(dim),
					lowByte(power_status),
					highByte(power_status),
				};
				for (int i = 0; i < sizeof(bulbLabel); i++)
				{
					StateData[i + 12] = bulbLabel[i];
				}
				for (int j = 0; j < sizeof(bulbTags); j++)
				{
					StateData[j + 12 + 32] = bulbTags[j];
				}
				memcpy(response.data, StateData, sizeof(StateData));
				response.data_size = sizeof(StateData);
				sendPacket(response, packet);
			}
		}
		break;

		case SET_WAVEFORM:
		{
			hue = word(request.data[3], request.data[2]);
			sat = word(request.data[5], request.data[4]);
			bri = word(request.data[7], request.data[6]);
			kel = word(request.data[9], request.data[8]);
			dur = (uint32_t)request.data[10] << 0 |
				  (uint32_t)request.data[11] << 8 |
				  (uint32_t)request.data[12] << 16 |
				  (uint32_t)request.data[13] << 24;
			setLight();
		}
		break;

		case SET_WAVEFORM_OPTIONAL:
		{
			if (request.data[21])
			{
				debug_print(F(" set hue "));
				hue = word(request.data[3], request.data[2]);
			}
			if (request.data[22])
			{
				debug_print(F(" set sat "));
				sat = word(request.data[5], request.data[4]);
			}
			if (request.data[23])
			{
				debug_print(F(" set bri "));
				bri = word(request.data[7], request.data[6]);
			}
			if (request.data[24])
			{
				debug_print(F(" set kel "));
				kel = word(request.data[9], request.data[8]);
			}
			dur = (uint32_t)request.data[10] << 0 |
				  (uint32_t)request.data[11] << 8 |
				  (uint32_t)request.data[12] << 16 |
				  (uint32_t)request.data[13] << 24;
			setLight();
		}
		break;

		case GET_LIGHT_STATE:
		{
			response.res_ack = NO_RESPONSE;
			response.packet_type = LIGHT_STATUS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			byte StateData[52] = {
				lowByte(hue),
				highByte(hue),
				lowByte(sat),
				highByte(sat),
				lowByte(bri),
				highByte(bri),
				lowByte(kel),
				highByte(kel),
				lowByte(dim),
				highByte(dim),
				lowByte(power_status),
				highByte(power_status),
			};
			for (int i = 0; i < sizeof(bulbLabel); i++)
			{
				StateData[i + 12] = bulbLabel[i];
			}
			for (int j = 0; j < sizeof(bulbTags); j++)
			{
				StateData[j + 12 + 32] = bulbTags[j];
			}
			memcpy(response.data, StateData, sizeof(StateData));
			response.data_size = sizeof(StateData);
			sendPacket(response, packet);
		}
		break;

		// this is a light strip call. Currently hardcoded for single bulb response
		case GET_COLOR_ZONE:
		{
			response.packet_type = STATE_COLOR_ZONE;
			response.protocol = LifxProtocol_BulbCommand;
			byte StateData[10] = {
				0x01,
				0x00,
				lowByte(hue),
				highByte(hue),
				lowByte(sat),
				highByte(sat),
				lowByte(bri),
				highByte(bri),
				lowByte(kel),
				highByte(kel),
			};
			memcpy(response.data, StateData, sizeof(StateData));
			response.data_size = sizeof(StateData);
			sendPacket(response, packet);
		}
		break;

		case SET_POWER_STATE:
		case SET_POWER_STATE2:
		{
			power_status = word(request.data[1], request.data[0]);
			setLight();
		}
		break;

		case GET_POWER_STATE:
		case GET_POWER_STATE2:
		{
			response.packet_type = POWER_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			byte PowerData[] = {
				lowByte(power_status),
				highByte(power_status)};

			memcpy(response.data, PowerData, sizeof(PowerData));
			response.data_size = sizeof(PowerData);
			sendPacket(response, packet);
		}
		break;

		case SET_BULB_LABEL:
		{
			for (int i = 0; i < LifxBulbLabelLength; i++)
			{
				if (bulbLabel[i] != request.data[i])
				{
					bulbLabel[i] = request.data[i];
				}
			}
		}
		break;

		case GET_BULB_LABEL:
		{
			response.packet_type = BULB_LABEL;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, bulbLabel, sizeof(bulbLabel));
			response.data_size = sizeof(bulbLabel);
			sendPacket(response, packet);
		}
		break;

		case SET_BULB_TAGS:
		case GET_BULB_TAGS:
		{
			if (request.packet_type == SET_BULB_TAGS)
			{
				for (int i = 0; i < LifxBulbTagsLength; i++)
				{
					if (bulbTags[i] != request.data[i])
					{
						bulbTags[i] = lowByte(request.data[i]);
					}
				}
			}

			response.packet_type = BULB_TAGS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, bulbTags, sizeof(bulbTags));
			response.data_size = sizeof(bulbTags);
			sendPacket(response, packet);
		}
		break;

		case SET_BULB_TAG_LABELS:
		case GET_BULB_TAG_LABELS:
		{
			if (request.packet_type == SET_BULB_TAG_LABELS)
			{
				for (int i = 0; i < LifxBulbTagLabelsLength; i++)
				{
					if (bulbTagLabels[i] != request.data[i])
					{
						bulbTagLabels[i] = request.data[i];
					}
				}
			}

			response.packet_type = BULB_TAG_LABELS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, bulbTagLabels, sizeof(bulbTagLabels));
			response.data_size = sizeof(bulbTagLabels);
			sendPacket(response, packet);
		}
		break;

		case SET_LOCATION_STATE:
		{
			for (int i = 0; i < 16; i++)
			{
				bulbLocationGUIDb[guidSeq[i]] = request.data[i];
			}
			for (int j = 0; j < 32; j++)
			{
				bulbLocation[j] = request.data[j + 16];
			}
			if (request.data[16 + 32] == 0)
			{
				auto time = this->ha_time_->utcnow();
				bulbLocationTime = (uint64_t)(((uint64_t)time.timestamp * 1000) * 1000000 + LifxMagicNum);
			}
			else
			{
				uint8_t *p = (uint8_t *)&bulbLocationTime;
				for (int k = 0; k < 8; k++)
				{
					p[k] = request.data[k + 32 + 16];
				}
			}
		}
		break;

		case GET_LOCATION_STATE:
		{
			response.packet_type = LOCATION_STATE;
			response.res_ack = RES_REQUIRED;
			response.protocol = LifxProtocol_AllBulbsResponse;
			uint8_t *p = (uint8_t *)&bulbLocationTime;
			byte LocationStateResponse[56] = {};
			for (int i = 0; i < sizeof(bulbLocationGUIDb); i++)
			{
				LocationStateResponse[i] = bulbLocationGUIDb[guidSeq[i]];
			}
			for (int j = 0; j < sizeof(bulbLocation); j++)
			{
				LocationStateResponse[j + 16] = bulbLocation[j];
			}
			for (int k = 0; k < sizeof(bulbLocationTime); k++)
			{
				LocationStateResponse[k + 32 + 16] = p[k];
			}

			memcpy(response.data, LocationStateResponse, sizeof(LocationStateResponse));
			response.data_size = sizeof(LocationStateResponse);
			sendPacket(response, packet);
		}
		break;

		case SET_AUTH_STATE:
		case GET_AUTH_STATE:
		{
			if (request.packet_type == SET_AUTH_STATE)
			{
				for (int i = 0; i < request.data_size; i++)
				{
					authResponse[i] = request.data[i];
				}
			}
			response.packet_type = AUTH_STATE;
			response.res_ack = RES_REQUIRED;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, authResponse, sizeof(authResponse));
			response.data_size = sizeof(authResponse);
			sendPacket(response, packet);
		}
		break;

		case SET_GROUP_STATE:
		{
			for (int i = 0; i < 16; i++)
			{
				bulbGroupGUIDb[guidSeq[i]] = request.data[i];
			}
			for (int j = 0; j < 32; j++)
			{
				bulbGroup[j] = request.data[j + 16];
			}
			if (request.data[16 + 32] == 0)
			{
				auto time = this->ha_time_->utcnow();
				bulbGroupTime = (uint64_t)(((uint64_t)time.timestamp * 1000) * 1000000 + LifxMagicNum);
			}
			else
			{
				uint8_t *p = (uint8_t *)&bulbGroupTime;
				for (int k = 0; k < 8; k++)
				{
					p[k] = request.data[k + 32 + 16];
				}
			}
		}
		break;

		case GET_GROUP_STATE:
		{
			response.packet_type = GROUP_STATE;
			response.res_ack = RES_REQUIRED;
			response.protocol = LifxProtocol_AllBulbsResponse;
			uint8_t *p = (uint8_t *)&bulbGroupTime;
			byte groupStateResponse[56] = {};
			for (int i = 0; i < sizeof(bulbGroupGUIDb); i++)
			{
				groupStateResponse[i] = bulbGroupGUIDb[guidSeq[i]];
			}
			for (int j = 0; j < sizeof(bulbGroup); j++)
			{
				groupStateResponse[j + 16] = bulbGroup[j];
			}
			for (int k = 0; k < sizeof(bulbGroupTime); k++)
			{
				groupStateResponse[k + 32 + 16] = p[k];
			}

			memcpy(response.data, groupStateResponse, sizeof(groupStateResponse));
			response.data_size = sizeof(groupStateResponse);
			sendPacket(response, packet);
		}
		break;

		case GET_VERSION_STATE:
		{
			response.packet_type = VERSION_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			response.res_ack = RES_REQUIRED;
			byte VersionData[] = {
				lowByte(LifxBulbVendor),
				highByte(LifxBulbVendor),
				0x00,
				0x00,
				lowByte(LifxBulbProduct),
				highByte(LifxBulbProduct),
				0x00,
				0x00,
				lowByte(LifxBulbVersion),
				highByte(LifxBulbVersion),
				0x00,
				0x00};

			memcpy(response.data, VersionData, sizeof(VersionData));
			response.data_size = sizeof(VersionData);
			sendPacket(response, packet);
		}
		break;

		case GET_MESH_FIRMWARE_STATE:
		{
			response.packet_type = MESH_FIRMWARE_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			response.res_ack = RES_REQUIRED;
			byte MeshVersionData[] = {
				0x00, 0x94, 0x18, 0x58, 0x1c, 0x05, 0xd9, 0x14,
				0x00, 0x94, 0x18, 0x58, 0x1c, 0x05, 0xd9, 0x14,
				0x16, 0x00, 0x01, 0x00
			};

			memcpy(response.data, MeshVersionData, sizeof(MeshVersionData));
			response.data_size = sizeof(MeshVersionData);
			sendPacket(response, packet);
		}
		break;

		case GET_WIFI_FIRMWARE_STATE:
		{
			response.packet_type = WIFI_FIRMWARE_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			response.res_ack = RES_REQUIRED;
			byte WifiVersionData[] = {
				0x00, 0x88, 0x82, 0xaa, 0x7d, 0x15, 0x35, 0x14,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x3e, 0x00, 0x65, 0x00
			};

			memcpy(response.data, WifiVersionData, sizeof(WifiVersionData));
			response.data_size = sizeof(WifiVersionData);
			sendPacket(response, packet);
		}
		break;

		case GET_WIFI_INFO:
		{
			response.packet_type = WIFI_INFO;
			response.protocol = LifxProtocol_AllBulbsResponse;
			float rssi = pow(10, (WiFi.RSSI() / 10));
			debug_print("RSSI: ");
			debug_println(rssi, DEC);

			byte *rssi_p = (byte *)&rssi;
			byte *tx_p = (byte *)&tx_bytes;
			byte *rx_p = (byte *)&rx_bytes;

			byte wifiInfo[] = {
				rssi_p[0],
				rssi_p[1],
				rssi_p[2],
				rssi_p[3],
				tx_p[0],
				tx_p[1],
				tx_p[2],
				tx_p[3],
				rx_p[0],
				rx_p[1],
				rx_p[2],
				rx_p[3],
				0x00,
				0x00};
			memcpy(response.data, wifiInfo, sizeof(wifiInfo));
			response.data_size = sizeof(wifiInfo);
			sendPacket(response, packet);
		}
		break;

		case SET_CLOUD_STATE:
		{
			cloudStatus = request.data[0];
			debug_print("Cloud status changed to: ");
			debug_println(cloudStatus, DEC);
		}
		break;

		case GET_CLOUD_STATE:
		{
			response.res_ack = RES_REQUIRED;
			response.packet_type = CLOUD_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			response.data[0] = cloudStatus;
			response.data_size = sizeof(cloudStatus);
			sendPacket(response, packet);
		}
		break;

		case SET_CLOUD_AUTH:
		case GET_CLOUD_AUTH:
		{
			if (request.packet_type == SET_CLOUD_AUTH)
			{
				for (int i = 0; i < request.data_size; i++)
				{
					cloudAuthResponse[i] = request.data[i];
				}
			}
			response.res_ack = RES_REQUIRED;
			response.packet_type = CLOUD_AUTH_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, cloudAuthResponse, sizeof(cloudAuthResponse));
			response.data_size = sizeof(cloudAuthResponse);
			sendPacket(response, packet);
		}
		break;

		case GET_CLOUD_BROKER:
		case SET_CLOUD_BROKER:
		{
			if (request.packet_type == SET_CLOUD_BROKER)
			{
				for (int i = 0; i < request.data_size; i++)
				{
					cloudBrokerUrl[i] = request.data[i];
				}
			}
			response.res_ack = RES_REQUIRED;
			response.packet_type = CLOUD_BROKER_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, cloudBrokerUrl, sizeof(cloudBrokerUrl));
			response.data_size = sizeof(cloudBrokerUrl);
			sendPacket(response, packet);
		}
		break;

		case ECHO_REQUEST:
		{
			response.packet_type = ECHO_RESPONSE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, request.data, sizeof(request.data));
			response.data_size = request.data_size;
			sendPacket(response, packet);
		}
		break;

		case PAN_GATEWAY:
		case STATE_INFO:
		case LOCATION_STATE:
		case GROUP_STATE:
		case LIGHT_STATUS:
		case AUTH_STATE:
		case VERSION_STATE:
		case WIFI_FIRMWARE_STATE:
		case MESH_FIRMWARE_STATE:
		case STATE_COLOR_ZONE:
		case BULB_LABEL:
		{
			// Ignore response packets from other bulbs
			debug_println("Ignoring bulb response packet");
		}
		break;

		default:
		{
			ESP_LOGW(TAG, "Unknown packet type: 0x%02X/%d", request.packet_type, request.packet_type);
		}
		break;
		}

		// Handle RES/ACK flags
		switch (request.res_ack)
		{
		case NO_RESPONSE:
		{
			debug_println("No RES_ACK");
		}
		break;

		case RES_REQUIRED:
		{
			debug_println("Response Requested");
		}
		break;

		case RES_ACK_PAN_REQUIRED:
		case ACK_PAN_REQUIRED:
		case RES_ACK_REQUIRED:
		case ACK_REQUIRED:
		{
			debug_println("Acknowledgement Requested");
			response.packet_type = ACKNOWLEDGEMENT;
			response.res_ack = NO_RESPONSE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memset(response.data, 0, sizeof(response.data));
			response.data_size = 0;
			sendPacket(response, packet);
		}
		break;

		case PAN_REQUIRED:
		{
			debug_println("PAN Response Required?");
		}
		break;

		case RES_PAN_REQUIRED:
		{
			debug_println("RES & PAN Response Required?");
		}
		break;

		default:
		{
			debug_print("Unknown RES_ACK ");
			debug_println(request.res_ack, HEX);
		}
		}
	}

	unsigned int sendPacket(LifxPacket &pkt, AsyncUDPPacket &Udpi)
	{
		int totalSize = LifxPacketSize + pkt.data_size;

		auto time = this->ha_time_->utcnow();

		// generate a nanosecond epoch timestamp (msec * 1,000,000 + magic)
		uint64_t packetT = (uint64_t)(((uint64_t)time.timestamp * 1000) * 1000000 + LifxMagicNum);
		uint8_t *packetTime = (uint8_t *)&packetT;

		uint8_t _message[totalSize + 1];
		int _packetLength = 0;

		memset(_message, 0, totalSize + 1);

		//// FRAME
		_message[_packetLength++] = (lowByte(totalSize));
		_message[_packetLength++] = (highByte(totalSize));
		_message[_packetLength++] = (lowByte(pkt.protocol));
		_message[_packetLength++] = (highByte(pkt.protocol));
		_message[_packetLength++] = pkt.source[0];
		_message[_packetLength++] = pkt.source[1];
		_message[_packetLength++] = pkt.source[2];
		_message[_packetLength++] = pkt.source[3];

		//// FRAME ADDRESS
		for (int i = 0; i < sizeof(mac); i++)
		{
			_message[_packetLength++] = mac[i];
		}
		// padding MAC
		_message[_packetLength++] = 0x00;
		_message[_packetLength++] = 0x00;
		// site mac address (LIFXV2)
		for (int i = 0; i < sizeof(site_mac); i++)
		{
			_message[_packetLength++] = site_mac[i];
		}
		_message[_packetLength++] = pkt.res_ack;
		_message[_packetLength++] = pkt.sequence;

		//// PROTOCOL HEADER
		_message[_packetLength++] = packetTime[0];
		_message[_packetLength++] = packetTime[1];
		_message[_packetLength++] = packetTime[2];
		_message[_packetLength++] = packetTime[3];
		_message[_packetLength++] = packetTime[4];
		_message[_packetLength++] = packetTime[5];
		_message[_packetLength++] = packetTime[6];
		_message[_packetLength++] = packetTime[7];
		_message[_packetLength++] = (lowByte(pkt.packet_type));
		_message[_packetLength++] = (highByte(pkt.packet_type));
		_message[_packetLength++] = 0x00;
		_message[_packetLength++] = 0x00;

		//data
		for (int i = 0; i < pkt.data_size; i++)
		{
			_message[_packetLength++] = pkt.data[i];
		}

		tx_bytes += _packetLength;

		Udpi.write(_message, _packetLength);

		if (debug_) ESP_LOGD(TAG, "Sent Packet Type 0x%02X (%d, %d bytes)", pkt.packet_type, pkt.packet_type, _packetLength);
		debug_print(F("Sent Packet Type 0x"));
		debug_print(pkt.packet_type, HEX);
		debug_print(F(" ("));
		debug_print(pkt.packet_type, DEC);
		debug_print(F(", "));
		debug_print(_packetLength);
		debug_print(F(" bytes): "));

		for (int j = 0; j < _packetLength; j++)
		{
			if (_message[j] <= 0x0F)
			{
				debug_print(F("0"));
			}
			debug_print(_message[j], HEX);
			debug_print(SPACE);
		}
		debug_println();
		return _packetLength;
	}

	// Sets the lights based on current HSBK + power state
	// Supports: combined RGBWW light OR separate CWWW + RGB lights
	void setLight()
	{
		int maxColor = 255;
		unsigned long loopRate = millis() - lastChange;
		debug_print(F("Packet rate:"));
		debug_print(loopRate);
		debug_println(F("msec"));
		debug_print(F("Set light - hue: "));
		debug_print(hue);
		debug_print(F(", sat: "));
		debug_print(sat);
		debug_print(F(", bri: "));
		debug_print(bri);
		debug_print(F(", kel: "));
		debug_print(kel);
		debug_print(F(", dur: "));
		debug_print(dur);
		debug_print(F(", power: "));
		debug_println(power_status ? " (on)" : "(off)");
		if (debug_) ESP_LOGD(TAG, "Set light - hue: %u, sat: %u, bri: %u, kel: %u, dur: %u, power: %s",
			hue, sat, bri, kel, dur, power_status ? "on" : "off");

		if (is_combined_mode())
		{
			setLightCombined();
		}
		else
		{
			setLightDual();
		}
		lastChange = millis();
	}

	// Combined RGBWW mode: single light with RGB + cold/warm white channels
	void setLightCombined()
	{
		int maxColor = 255;

		if (power_status && bri)
		{
			float bright = (float)bri / 65535;
			auto call = this->rgbww_led_->turn_on();

			if (sat < 1)
			{
				uint16_t mireds = 1000000 / kel;
				call.set_color_temperature(mireds);
			}
			else
			{
				uint8_t rgbColor[3];
				int this_hue = map(hue, 0, 65535, 0, 767);
				int this_sat = map(sat, 0, 65535, 0, 255);
				int this_bri = map(bri, 0, 65535, 0, 255);

				hsb2rgb(this_hue, this_sat, this_bri, rgbColor);
				float r = (float)rgbColor[0] / maxColor;
				float g = (float)rgbColor[1] / maxColor;
				float b = (float)rgbColor[2] / maxColor;

				call.set_rgb(r, g, b);
				call.set_cold_white(0.0f);
				call.set_warm_white(0.0f);
			}

			call.set_brightness(bright);
			if (dur > lastChange)
			{
				call.set_transition_length(0);
			}
			else
			{
				call.set_transition_length(dur);
			}
			call.perform();
		}
		else
		{
			auto call = this->rgbww_led_->turn_off();
			call.set_brightness(0);
			call.set_transition_length(dur);
			call.perform();
		}
	}

	// Dual mode: separate RGB color light + CWWW white light
	void setLightDual()
	{
		int maxColor = 255;

		if (power_status && bri)
		{
			float bright = (float)bri / 65535;

			if (sat < 1)
			{
				auto callC = this->color_led_->turn_off();
				callC.perform();

				auto callW = this->white_led_->turn_on();
				uint16_t mireds = 1000000 / kel;
				callW.set_color_temperature(mireds);
				callW.set_brightness(bright);

				if (dur > lastChange)
				{
					callW.set_transition_length(0);
				}
				else
				{
					callW.set_transition_length(dur);
				}
				callW.perform();
			}
			else
			{
				uint8_t rgbColor[3];
				auto callW = this->white_led_->turn_off();
				auto callC = this->color_led_->turn_on();

				int this_hue = map(hue, 0, 65535, 0, 767);
				int this_sat = map(sat, 0, 65535, 0, 255);
				int this_bri = map(bri, 0, 65535, 0, 255);

				hsb2rgb(this_hue, this_sat, this_bri, rgbColor);
				float r = (float)rgbColor[0] / maxColor;
				float g = (float)rgbColor[1] / maxColor;
				float b = (float)rgbColor[2] / maxColor;

				callC.set_rgb(r, g, b);
				callC.set_brightness(bright);
				callC.set_transition_length(dur);
				callW.perform();
				callC.perform();
			}
		}
		else
		{
			auto callC = this->color_led_->turn_off();
			callC.set_brightness(0);
			callC.set_transition_length(dur);
			callC.perform();

			auto callW = this->white_led_->turn_off();
			callW.set_brightness(0);
			callW.set_transition_length(dur);
			callW.perform();
		}
	}

	void hexCharacterStringToBytes(byte *byteArray, const char *hexString)
	{
		bool oddLength = strlen(hexString) & 1;

		byte currentByte = 0;
		byte byteIndex = 0;
		byte removed = 0;

		for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
		{
			bool oddCharIndex = (charIndex - removed) & 1;
			if (hexString[charIndex] == '-')
			{
				removed++;
				continue;
			}

			if (oddLength)
			{
				if (oddCharIndex)
				{
					currentByte = nibble(hexString[charIndex]) << 4;
				}
				else
				{
					currentByte |= nibble(hexString[charIndex]);
					byteArray[byteIndex++] = currentByte;
					currentByte = 0;
				}
			}
			else
			{
				if (!oddCharIndex)
				{
					currentByte = nibble(hexString[charIndex]) << 4;
				}
				else
				{
					currentByte |= nibble(hexString[charIndex]);
					byteArray[byteIndex++] = currentByte;
					currentByte = 0;
				}
			}
		}
	}

	byte nibble(char c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		return 0;
	}

	/******************************************************************************
	 * HSB to RGB conversion.
	 * hue (index): 0-767, sat and bright: 0-255, color[]: output RGB bytes
	 *****************************************************************************/
	void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3])
	{
		uint16_t r_temp, g_temp, b_temp;
		uint8_t index_mod;
		uint8_t inverse_sat = (sat ^ 255);

		index = index % 768;
		index_mod = index % 256;

		if (index < 256)
		{
			r_temp = index_mod ^ 255;
			g_temp = index_mod;
			b_temp = 0;
		}
		else if (index < 512)
		{
			r_temp = 0;
			g_temp = index_mod ^ 255;
			b_temp = index_mod;
		}
		else if (index < 768)
		{
			r_temp = index_mod;
			g_temp = 0;
			b_temp = index_mod ^ 255;
		}
		else
		{
			r_temp = 0;
			g_temp = 0;
			b_temp = 0;
		}

		r_temp = ((r_temp * sat) / 255) + inverse_sat;
		g_temp = ((g_temp * sat) / 255) + inverse_sat;
		b_temp = ((b_temp * sat) / 255) + inverse_sat;

		r_temp = (r_temp * bright) / 255;
		g_temp = (g_temp * bright) / 255;
		b_temp = (b_temp * bright) / 255;

		color[0] = (uint8_t)r_temp;
		color[1] = (uint8_t)g_temp;
		color[2] = (uint8_t)b_temp;
	}
};

} // namespace lifx_emulation
} // namespace esphome
