#pragma once

#include <cstdint>
#include <Arduino.h>

// LIFX LAN Protocol - In-memory packet representation
// Reference: https://lan.developer.lifx.com/docs/packet-contents
//
// Wire format header (36 bytes):
//   Frame (8 bytes):
//     [0-1]   size        uint16 LE - total message size including header
//     [2-3]   protocol    uint16 LE - bits 0-11: protocol (1024)
//                                     bit 12: addressable (must be 1)
//                                     bit 13: tagged (1=broadcast, 0=directed)
//                                     bits 14-15: origin (must be 0)
//     [4-7]   source      uint32 - unique client identifier
//   Frame Address (16 bytes):
//     [8-13]  target      6 bytes MAC (or all zeros for broadcast)
//     [14-21] reserved    6+2 bytes
//     [22]    flags       bit 0: res_required, bit 1: ack_required
//     [23]    sequence    uint8 - message sequence number
//   Protocol Header (12 bytes):
//     [24-31] reserved    uint64
//     [32-33] type        uint16 LE - message type
//     [34-35] reserved    uint16
//
// This struct is NOT a wire-format overlay; fields are populated via
// manual deserialization in processRequest().
struct LifxPacket
{
	// Frame
	uint16_t size;       // total message size (LE)
	uint16_t protocol;   // protocol(1024) | addressable | tagged | origin
	byte source[4];      // client source identifier

	// Frame Address
	byte bulbAddress[6]; // target device MAC
	uint16_t reserved2;  // reserved
	byte site[6];        // reserved

	uint8_t res_ack;     // bit 0: res_required, bit 1: ack_required
	uint8_t sequence;    // message sequence number

	// Protocol Header
	uint64_t timestamp;  // reserved
	uint16_t packet_type; // message type (LE)
	uint16_t reserved4;

	// Payload (not part of wire header)
	byte data[128];
	int data_size;
};

// Protocol field values (protocol | addressable | tagged | origin bits)
// Protocol is always 1024 (0x400), addressable is always 1 (bit 12)
const unsigned int LifxProtocol_AllBulbsResponse = 21504; // 0x5400 - origin=1, tagged=0
const unsigned int LifxProtocol_AllBulbsRequest = 13312;  // 0x3400 - origin=0, tagged=1
const unsigned int LifxProtocol_BulbCommand = 5120;       // 0x1400 - origin=0, tagged=0

const unsigned int LifxPacketSize = 36;
const unsigned int LifxPort = 56700; // local port to listen on
const unsigned int LifxBulbLabelLength = 32;
const unsigned int LifxBulbTagsLength = 8;
const unsigned int LifxBulbTagLabelsLength = 32;
#define LIFX_MAX_PACKET_LENGTH 512

// Firmware versions, etc
const byte LifxBulbVendor = 1;
const byte LifxBulbProduct = 22;
const byte LifxBulbVersion = 0;
const byte LifxFirmwareVersionMajor = 1;
const byte LifxFirmwareVersionMinor = 5;
const unsigned int LifxMagicNum = 614500;

// Service types (StateService payload)
const byte SERVICE_UDP = 0x01;
const byte SERVICE_TCP = 0x02;
const byte SERVICE_UDP5 = 0x05; // Real bulbs also offer this service

// Packet RES/ACK flags (byte 22 of header)
// bit 0: res_required, bit 1: ack_required, bits 2-7: reserved
const byte NO_RESPONSE = 0x00;
const byte RES_REQUIRED = 0x01;
const byte ACK_REQUIRED = 0x02;
const byte RES_ACK_REQUIRED = 0x03;    // both res + ack
const byte PAN_REQUIRED = 0x04;        // observed on GET_PAN_GATEWAY (bit 2)
const byte RES_PAN_REQUIRED = 0x05;    // bits 0+2
const byte ACK_PAN_REQUIRED = 0x06;    // bits 1+2
const byte RES_ACK_PAN_REQUIRED = 0x07; // bits 0+1+2

// ============================================================================
// Device Messages (0-99)
// https://lan.developer.lifx.com/docs/querying-the-device
// https://lan.developer.lifx.com/docs/changing-a-device
// https://lan.developer.lifx.com/docs/information-messages
// ============================================================================

const byte GET_PAN_GATEWAY = 0x02;        // GetService(2)
const byte PAN_GATEWAY = 0x03;            // StateService(3)

const byte GET_HOST_INFO = 0x0c;          // GetHostInfo(12)
const byte HOST_INFO = 0x0d;              // StateHostInfo(13)

const byte GET_MESH_FIRMWARE_STATE = 0x0e; // GetHostFirmware(14)
const byte MESH_FIRMWARE_STATE = 0x0f;     // StateHostFirmware(15)

const byte GET_WIFI_INFO = 0x10;          // GetWifiInfo(16)
const byte WIFI_INFO = 0x11;              // StateWifiInfo(17)

const byte GET_WIFI_FIRMWARE_STATE = 0x12; // GetWifiFirmware(18)
const byte WIFI_FIRMWARE_STATE = 0x13;     // StateWifiFirmware(19)

const byte GET_POWER_STATE = 0x14;        // GetPower(20)
const byte SET_POWER_STATE = 0x15;        // SetPower(21)
const byte POWER_STATE = 0x16;            // StatePower(22)

const byte GET_BULB_LABEL = 0x17;         // GetLabel(23)
const byte SET_BULB_LABEL = 0x18;         // SetLabel(24)
const byte BULB_LABEL = 0x19;             // StateLabel(25)

const byte GET_BULB_TAGS = 0x1a;          // undocumented (26)
const byte SET_BULB_TAGS = 0x1b;          // undocumented (27)
const byte BULB_TAGS = 0x1c;              // undocumented (28)

const byte GET_BULB_TAG_LABELS = 0x1d;    // undocumented (29)
const byte SET_BULB_TAG_LABELS = 0x1e;    // undocumented (30)
const byte BULB_TAG_LABELS = 0x1f;        // undocumented (31)

const byte GET_VERSION_STATE = 0x20;      // GetVersion(32)
const byte VERSION_STATE = 0x21;          // StateVersion(33)

const byte GET_INFO = 0x22;               // GetInfo(34)
const byte STATE_INFO = 0x23;             // StateInfo(35)

const byte SET_REBOOT = 0x26;             // SetReboot(38) - no payload, no response

// NOTE: RESET_BULB_ANDROID was previously 0x37 which conflicts with SET_AUTH_STATE.
// Observed android reset traffic was likely SetAuth(55) packets, not a separate type.

const byte ACKNOWLEDGEMENT = 0x2d;        // Acknowledgement(45)
const byte RESET_BULB = 0x2e;             // observed on bulb reset (46)

const byte GET_LOCATION_STATE = 0x30;     // GetLocation(48)
const byte SET_LOCATION_STATE = 0x31;     // SetLocation(49)
const byte LOCATION_STATE = 0x32;         // StateLocation(50)

const byte GET_GROUP_STATE = 0x33;        // GetGroup(51)
const byte SET_GROUP_STATE = 0x34;        // SetGroup(52)
const byte GROUP_STATE = 0x35;            // StateGroup(53)

const byte GET_AUTH_STATE = 0x36;         // undocumented (54) - suspected FW checksum
const byte SET_AUTH_STATE = 0x37;         // undocumented (55)
const byte AUTH_STATE = 0x38;             // undocumented (56)

const byte ECHO_REQUEST = 0x3a;           // EchoRequest(58)
const byte ECHO_RESPONSE = 0x3b;          // EchoResponse(59)

// ============================================================================
// Light Messages (100-199)
// ============================================================================

const byte GET_LIGHT_STATE = 0x65;        // LightGet(101)
const byte SET_LIGHT_STATE = 0x66;        // LightSetColor(102)
const byte SET_WAVEFORM = 0x67;           // LightSetWaveform(103)

const byte LIGHT_STATUS = 0x6b;           // LightState(107)

const byte GET_POWER_STATE2 = 0x74;       // LightGetPower(116)
const byte SET_POWER_STATE2 = 0x75;       // LightSetPower(117)
const byte POWER_STATE2 = 0x76;           // LightStatePower(118)

const byte SET_WAVEFORM_OPTIONAL = 0x77;  // LightSetWaveformOptional(119)

const byte GET_INFARED_STATE = 0x78;      // GetInfrared(120)
const byte STATE_INFARED_STATE = 0x79;    // StateInfrared(121)
const byte SET_INFARED_STATE = 0x7A;      // SetInfrared(122)

// HEV (High Energy Visible light / Clean cycle) Messages
const byte GET_HEV_CYCLE = 0x8E;          // GetHevCycle(142)
const byte SET_HEV_CYCLE = 0x8F;          // SetHevCycle(143)
const byte STATE_HEV_CYCLE = 0x90;        // StateHevCycle(144)
const byte GET_HEV_CYCLE_CONFIG = 0x91;   // GetHevCycleConfiguration(145)
const byte SET_HEV_CYCLE_CONFIG = 0x92;   // SetHevCycleConfiguration(146)
const byte STATE_HEV_CYCLE_CONFIG = 0x93; // StateHevCycleConfiguration(147)
const byte GET_LAST_HEV_RESULT = 0x94;    // GetLastHevCycleResult(148)
const byte STATE_LAST_HEV_RESULT = 0x95;  // StateLastHevCycleResult(149)

// ============================================================================
// Cloud Messages (200-299) - undocumented, observed from real devices
// ============================================================================

const byte GET_CLOUD_STATE = 0xc9;        // (201)
const byte SET_CLOUD_STATE = 0xca;        // (202)
const byte CLOUD_STATE = 0xcb;            // (203)

const byte GET_CLOUD_AUTH = 0xcc;         // (204)
const byte SET_CLOUD_AUTH = 0xcd;         // (205)
const byte CLOUD_AUTH_STATE = 0xce;       // (206)

const byte GET_CLOUD_BROKER = 0xd1;       // (209)
const byte SET_CLOUD_BROKER = 0xd2;       // (210)
const byte CLOUD_BROKER_STATE = 0xd3;     // (211)

const byte STATE_UNHANDLED = 0xdf;        // StateUnhandled(223)

// ============================================================================
// MultiZone Messages (500-599)
// https://lan.developer.lifx.com/docs/changing-a-device#multizone-messages
// ============================================================================

const uint16_t SET_COLOR_ZONES = 501;          // SetColorZones(501)
const uint16_t GET_COLOR_ZONE = 502;           // GetColorZones(502)
const uint16_t STATE_COLOR_ZONE = 503;         // StateZone(503)
const uint16_t STATE_MULTI_ZONE = 506;         // StateMultiZone(506)
const uint16_t GET_MULTI_ZONE_EFFECT = 507;    // GetMultiZoneEffect(507)
const uint16_t SET_MULTI_ZONE_EFFECT = 508;    // SetMultiZoneEffect(508)
const uint16_t STATE_MULTI_ZONE_EFFECT = 509;  // StateMultiZoneEffect(509)
const uint16_t SET_EXT_COLOR_ZONES = 510;      // SetExtendedColorZones(510)
const uint16_t GET_EXT_COLOR_ZONES = 511;      // GetExtendedColorZones(511)
const uint16_t STATE_EXT_COLOR_ZONES = 512;    // StateExtendedColorZones(512)

// ============================================================================
// Tile Messages (700-799)
// ============================================================================

const uint16_t GET_DEVICE_CHAIN = 701;         // GetDeviceChain(701)
const uint16_t STATE_DEVICE_CHAIN = 702;       // StateDeviceChain(702)
const uint16_t SET_USER_POSITION = 703;        // SetUserPosition(703)
const uint16_t GET_TILE_STATE64 = 707;         // Get64(707)
const uint16_t STATE_TILE_STATE64 = 711;       // State64(711)
const uint16_t SET_TILE_STATE64 = 715;         // Set64(715)
const uint16_t SET_TILE_BUFFER_COPY = 716;     // CopyFrameBuffer(716) (FW 4.4+)
const uint16_t GET_TILE_EFFECT = 718;          // GetTileEffect(718)
const uint16_t SET_TILE_EFFECT = 719;          // SetTileEffect(719)
const uint16_t STATE_TILE_EFFECT = 720;        // StateTileEffect(720)

// ============================================================================
// Relay Messages (800-899) - LIFX Switch
// ============================================================================

const uint16_t GET_RPOWER = 816;               // GetRPower(816)
const uint16_t SET_RPOWER = 817;               // SetRPower(817)
const uint16_t STATE_RPOWER = 818;             // StateRPower(818)

// ============================================================================
// Enumerations
// https://lan.developer.lifx.com/docs/waveforms
// ============================================================================

enum LifxWaveform : uint8_t {
	WAVEFORM_SAW       = 0,
	WAVEFORM_SINE      = 1,
	WAVEFORM_HALF_SINE = 2,
	WAVEFORM_TRIANGLE  = 3,
	WAVEFORM_PULSE     = 4,
};

enum LifxMultiZoneApply : uint8_t {
	APPLY_NO_APPLY  = 0, // don't apply yet
	APPLY_APPLY     = 1, // apply immediately
	APPLY_APPLY_ONLY = 2, // apply without changing zone buffer
};

enum LifxMultiZoneEffectType : uint8_t {
	MULTIZONE_EFFECT_OFF  = 0,
	MULTIZONE_EFFECT_MOVE = 1,
};

enum LifxTileEffectType : uint8_t {
	TILE_EFFECT_OFF     = 0,
	TILE_EFFECT_MORPH   = 2,
	TILE_EFFECT_FLAME   = 3,
	TILE_EFFECT_SKY     = 5,
};

enum LifxLastHevResult : uint8_t {
	HEV_SUCCESS            = 0,
	HEV_BUSY               = 1,
	HEV_INTERRUPTED_BY_RESET = 2,
	HEV_INTERRUPTED_BY_HOMEKIT = 3,
	HEV_INTERRUPTED_BY_LAN = 4,
	HEV_INTERRUPTED_BY_CLOUD = 5,
	HEV_NONE               = 255,
};

// ============================================================================
// Payload Structures (packed, little-endian)
// These can be used for documentation or direct buffer interpretation.
// The current codebase uses manual byte-level access into LifxPacket.data[].
// ============================================================================

// HSBK color structure (8 bytes) - used in many payloads
struct __attribute__((packed)) LifxHSBK {
	uint16_t hue;        // 0-65535 (maps to 0-360 degrees)
	uint16_t saturation; // 0-65535
	uint16_t brightness; // 0-65535
	uint16_t kelvin;     // 1500-9000
};

// --- Device Message Payloads ---

// StateService(3) payload
struct __attribute__((packed)) LifxPayloadStateService {
	uint8_t service;     // SERVICE_UDP, SERVICE_TCP, etc.
	uint32_t port;       // typically 56700
};

// StateHostInfo(13) payload
struct __attribute__((packed)) LifxPayloadStateHostInfo {
	float signal;        // signal strength (mW)
	uint32_t tx;         // bytes transmitted
	uint32_t rx;         // bytes received
	int16_t reserved;
};

// StateHostFirmware(15) / StateWifiFirmware(19) payload
struct __attribute__((packed)) LifxPayloadStateFirmware {
	uint64_t build;      // firmware build timestamp (nanoseconds since epoch)
	uint64_t reserved;
	uint16_t version_minor;
	uint16_t version_major;
};

// StateWifiInfo(17) payload
struct __attribute__((packed)) LifxPayloadStateWifiInfo {
	float signal;        // signal strength (mW)
	uint32_t reserved6;
	uint32_t reserved7;
	int16_t reserved8;
};

// SetPower(21) / StatePower(22) payload
struct __attribute__((packed)) LifxPayloadPower {
	uint16_t level;      // 0 = off, 65535 = on
};

// SetLabel(24) / StateLabel(25) payload
struct __attribute__((packed)) LifxPayloadLabel {
	char label[32];      // null-padded UTF-8 string
};

// StateVersion(33) payload
struct __attribute__((packed)) LifxPayloadStateVersion {
	uint32_t vendor;     // LIFX = 1
	uint32_t product;    // see product registry
	uint32_t reserved;
};

// StateInfo(35) payload
struct __attribute__((packed)) LifxPayloadStateInfo {
	uint64_t time;       // current time (nanoseconds since epoch)
	uint64_t uptime;     // uptime (nanoseconds)
	uint64_t downtime;   // last downtime duration (nanoseconds)
};

// SetLocation(49) / StateLocation(50) payload
struct __attribute__((packed)) LifxPayloadLocation {
	byte location[16];   // UUID
	char label[32];      // null-padded UTF-8 string
	uint64_t updated_at; // nanoseconds since epoch
};

// SetGroup(52) / StateGroup(53) payload
struct __attribute__((packed)) LifxPayloadGroup {
	byte group[16];      // UUID
	char label[32];      // null-padded UTF-8 string
	uint64_t updated_at; // nanoseconds since epoch
};

// EchoRequest(58) / EchoResponse(59) payload
struct __attribute__((packed)) LifxPayloadEcho {
	byte echoing[64];    // arbitrary data echoed back
};

// StateUnhandled(223) payload
struct __attribute__((packed)) LifxPayloadStateUnhandled {
	uint16_t unhandled_type; // the message type that was not handled
};

// --- Light Message Payloads ---

// LightSetColor(102) payload
struct __attribute__((packed)) LifxPayloadSetColor {
	uint8_t reserved;
	uint16_t hue;
	uint16_t saturation;
	uint16_t brightness;
	uint16_t kelvin;
	uint32_t duration;   // transition time in milliseconds
};

// LightSetWaveform(103) payload
struct __attribute__((packed)) LifxPayloadSetWaveform {
	uint8_t reserved;
	uint8_t transient;   // 1 = return to original color after effect
	uint16_t hue;
	uint16_t saturation;
	uint16_t brightness;
	uint16_t kelvin;
	uint32_t period;     // milliseconds per cycle
	float cycles;        // number of cycles
	int16_t skew_ratio;  // -32768 to 32767 (PULSE duty cycle)
	uint8_t waveform;    // LifxWaveform enum
};

// LightState(107) payload
struct __attribute__((packed)) LifxPayloadLightState {
	uint16_t hue;
	uint16_t saturation;
	uint16_t brightness;
	uint16_t kelvin;
	int16_t reserved6;
	uint16_t power;
	char label[32];
	uint64_t reserved7;
};

// LightSetPower(117) / LightStatePower(118) payload
struct __attribute__((packed)) LifxPayloadLightPower {
	uint16_t level;      // 0 = off, 65535 = on
	uint32_t duration;   // transition time in milliseconds (only in SetPower)
};

// LightSetWaveformOptional(119) payload
struct __attribute__((packed)) LifxPayloadSetWaveformOptional {
	uint8_t reserved;
	uint8_t transient;
	uint16_t hue;
	uint16_t saturation;
	uint16_t brightness;
	uint16_t kelvin;
	uint32_t period;     // milliseconds per cycle
	float cycles;        // number of cycles
	int16_t skew_ratio;
	uint8_t waveform;    // LifxWaveform enum
	uint8_t set_hue;     // apply hue from this packet
	uint8_t set_saturation;
	uint8_t set_brightness;
	uint8_t set_kelvin;
};

// SetInfrared(122) / StateInfrared(121) payload
struct __attribute__((packed)) LifxPayloadInfrared {
	uint16_t brightness; // 0-65535
};

// --- HEV Message Payloads ---

// SetHevCycle(143) payload
struct __attribute__((packed)) LifxPayloadSetHevCycle {
	uint8_t enable;      // 0 = disable, 1 = enable
	uint32_t duration_s; // cycle duration in seconds
};

// StateHevCycle(144) payload
struct __attribute__((packed)) LifxPayloadStateHevCycle {
	uint32_t duration_s;
	uint32_t remaining_s;
	uint8_t last_power;  // power state before HEV cycle
};

// SetHevCycleConfiguration(146) / StateHevCycleConfiguration(147) payload
struct __attribute__((packed)) LifxPayloadHevCycleConfig {
	uint8_t indication;  // 1 = show visual indication during cycle
	uint32_t duration_s; // default duration in seconds
};

// StateLastHevCycleResult(149) payload
struct __attribute__((packed)) LifxPayloadLastHevResult {
	uint8_t result;      // LifxLastHevResult enum
};

// --- MultiZone Message Payloads ---

// SetColorZones(501) payload
struct __attribute__((packed)) LifxPayloadSetColorZones {
	uint8_t start_index;
	uint8_t end_index;
	LifxHSBK color;
	uint32_t duration;   // transition time in milliseconds
	uint8_t apply;       // LifxMultiZoneApply enum
};

// StateZone(503) payload
struct __attribute__((packed)) LifxPayloadStateZone {
	uint8_t zones_count;
	uint8_t zone_index;
	LifxHSBK color;
};

// StateMultiZone(506) payload
struct __attribute__((packed)) LifxPayloadStateMultiZone {
	uint8_t zones_count;
	uint8_t zone_index;
	LifxHSBK colors[8];
};

// SetMultiZoneEffect(508) / StateMultiZoneEffect(509) payload
struct __attribute__((packed)) LifxPayloadMultiZoneEffect {
	uint32_t instanceid;
	uint8_t type;        // LifxMultiZoneEffectType enum
	uint16_t reserved6;
	uint32_t speed;      // milliseconds
	uint64_t duration;   // nanoseconds
	uint32_t reserved7;
	uint32_t reserved8;
	byte parameters[32];
};

// SetExtendedColorZones(510) payload
struct __attribute__((packed)) LifxPayloadSetExtColorZones {
	uint32_t duration;
	uint8_t apply;       // LifxMultiZoneApply enum
	uint16_t zone_index;
	uint8_t colors_count;
	LifxHSBK colors[82];
};

// StateExtendedColorZones(512) payload
struct __attribute__((packed)) LifxPayloadStateExtColorZones {
	uint16_t zones_count;
	uint16_t zone_index;
	uint8_t colors_count;
	LifxHSBK colors[82];
};

// --- Relay Message Payloads (LIFX Switch) ---

// SetRPower(817) / StateRPower(818) payload
struct __attribute__((packed)) LifxPayloadRPower {
	uint8_t relay_index;
	uint16_t level;      // 0 = off, 65535 = on
};

// --- Tile Message Payloads ---

// SetUserPosition(703) payload
struct __attribute__((packed)) LifxPayloadSetUserPosition {
	uint8_t tile_index;
	uint16_t reserved;
	float user_x;
	float user_y;
};

// Set64(715) payload
struct __attribute__((packed)) LifxPayloadSet64 {
	uint8_t tile_index;
	uint8_t length;
	uint8_t fb_index;
	uint8_t x;
	uint8_t y;
	uint8_t width;
	uint32_t duration;
	LifxHSBK colors[64];
};

// State64(711) payload
struct __attribute__((packed)) LifxPayloadState64 {
	uint8_t tile_index;
	uint8_t reserved;
	uint8_t x;
	uint8_t y;
	uint8_t width;
	LifxHSBK colors[64];
};

// ============================================================================
// Packet type name lookup (for debug logging)
// ============================================================================

inline const char *lifx_packet_type_name(uint16_t type) {
	switch (type) {
	// Device messages
	case GET_PAN_GATEWAY: return "GetService";
	case PAN_GATEWAY: return "StateService";
	case GET_HOST_INFO: return "GetHostInfo";
	case HOST_INFO: return "StateHostInfo";
	case GET_MESH_FIRMWARE_STATE: return "GetHostFirmware";
	case MESH_FIRMWARE_STATE: return "StateHostFirmware";
	case GET_WIFI_INFO: return "GetWifiInfo";
	case WIFI_INFO: return "StateWifiInfo";
	case GET_WIFI_FIRMWARE_STATE: return "GetWifiFirmware";
	case WIFI_FIRMWARE_STATE: return "StateWifiFirmware";
	case GET_POWER_STATE: return "GetPower";
	case SET_POWER_STATE: return "SetPower";
	case POWER_STATE: return "StatePower";
	case GET_BULB_LABEL: return "GetLabel";
	case SET_BULB_LABEL: return "SetLabel";
	case BULB_LABEL: return "StateLabel";
	case GET_BULB_TAGS: return "GetTags";
	case SET_BULB_TAGS: return "SetTags";
	case BULB_TAGS: return "StateTags";
	case GET_BULB_TAG_LABELS: return "GetTagLabels";
	case SET_BULB_TAG_LABELS: return "SetTagLabels";
	case BULB_TAG_LABELS: return "StateTagLabels";
	case GET_VERSION_STATE: return "GetVersion";
	case VERSION_STATE: return "StateVersion";
	case GET_INFO: return "GetInfo";
	case STATE_INFO: return "StateInfo";
	case SET_REBOOT: return "SetReboot";
	case ACKNOWLEDGEMENT: return "Acknowledgement";
	case RESET_BULB: return "ResetBulb";
	case GET_LOCATION_STATE: return "GetLocation";
	case SET_LOCATION_STATE: return "SetLocation";
	case LOCATION_STATE: return "StateLocation";
	case GET_GROUP_STATE: return "GetGroup";
	case SET_GROUP_STATE: return "SetGroup";
	case GROUP_STATE: return "StateGroup";
	case GET_AUTH_STATE: return "GetAuth";
	case SET_AUTH_STATE: return "SetAuth";
	case AUTH_STATE: return "StateAuth";
	case ECHO_REQUEST: return "EchoRequest";
	case ECHO_RESPONSE: return "EchoResponse";
	// Light messages
	case GET_LIGHT_STATE: return "LightGet";
	case SET_LIGHT_STATE: return "LightSetColor";
	case SET_WAVEFORM: return "LightSetWaveform";
	case LIGHT_STATUS: return "LightState";
	case GET_POWER_STATE2: return "LightGetPower";
	case SET_POWER_STATE2: return "LightSetPower";
	case POWER_STATE2: return "LightStatePower";
	case SET_WAVEFORM_OPTIONAL: return "LightSetWaveformOptional";
	case GET_INFARED_STATE: return "GetInfrared";
	case STATE_INFARED_STATE: return "StateInfrared";
	case SET_INFARED_STATE: return "SetInfrared";
	// HEV messages
	case GET_HEV_CYCLE: return "GetHevCycle";
	case SET_HEV_CYCLE: return "SetHevCycle";
	case STATE_HEV_CYCLE: return "StateHevCycle";
	case GET_HEV_CYCLE_CONFIG: return "GetHevCycleConfiguration";
	case SET_HEV_CYCLE_CONFIG: return "SetHevCycleConfiguration";
	case STATE_HEV_CYCLE_CONFIG: return "StateHevCycleConfiguration";
	case GET_LAST_HEV_RESULT: return "GetLastHevCycleResult";
	case STATE_LAST_HEV_RESULT: return "StateLastHevCycleResult";
	// Cloud messages
	case GET_CLOUD_STATE: return "GetCloud";
	case SET_CLOUD_STATE: return "SetCloud";
	case CLOUD_STATE: return "StateCloud";
	case GET_CLOUD_AUTH: return "GetCloudAuth";
	case SET_CLOUD_AUTH: return "SetCloudAuth";
	case CLOUD_AUTH_STATE: return "StateCloudAuth";
	case GET_CLOUD_BROKER: return "GetCloudBroker";
	case SET_CLOUD_BROKER: return "SetCloudBroker";
	case CLOUD_BROKER_STATE: return "StateCloudBroker";
	case STATE_UNHANDLED: return "StateUnhandled";
	// MultiZone messages
	case SET_COLOR_ZONES: return "SetColorZones";
	case GET_COLOR_ZONE: return "GetColorZones";
	case STATE_COLOR_ZONE: return "StateZone";
	case STATE_MULTI_ZONE: return "StateMultiZone";
	case GET_MULTI_ZONE_EFFECT: return "GetMultiZoneEffect";
	case SET_MULTI_ZONE_EFFECT: return "SetMultiZoneEffect";
	case STATE_MULTI_ZONE_EFFECT: return "StateMultiZoneEffect";
	case SET_EXT_COLOR_ZONES: return "SetExtendedColorZones";
	case GET_EXT_COLOR_ZONES: return "GetExtendedColorZones";
	case STATE_EXT_COLOR_ZONES: return "StateExtendedColorZones";
	// Tile messages
	case GET_DEVICE_CHAIN: return "GetDeviceChain";
	case STATE_DEVICE_CHAIN: return "StateDeviceChain";
	case SET_USER_POSITION: return "SetUserPosition";
	case GET_TILE_STATE64: return "Get64";
	case STATE_TILE_STATE64: return "State64";
	case SET_TILE_STATE64: return "Set64";
	case SET_TILE_BUFFER_COPY: return "CopyFrameBuffer";
	case GET_TILE_EFFECT: return "GetTileEffect";
	case SET_TILE_EFFECT: return "SetTileEffect";
	case STATE_TILE_EFFECT: return "StateTileEffect";
	// Relay messages
	case GET_RPOWER: return "GetRPower";
	case SET_RPOWER: return "SetRPower";
	case STATE_RPOWER: return "StateRPower";
	default: return "Unknown";
	}
}
