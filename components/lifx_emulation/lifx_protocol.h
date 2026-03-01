#pragma once

#include <cstdint>
#include <Arduino.h>

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
