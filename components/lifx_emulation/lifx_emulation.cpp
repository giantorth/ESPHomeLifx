#include "lifx_emulation.h"
#include "lifx_utils.h"
#include <cmath>

namespace esphome {
namespace lifx_emulation {

static const char *const TAG = "lifx_emulation";

uint32_t LifxEmulation::compute_yaml_hash_()
{
	uint32_t h = fnv1_hash(bulbLabel);
	h ^= fnv1_hash(bulbLocation);
	h ^= fnv1_hash(bulbLocationGUID);
	h ^= fnv1_hash(bulbGroup);
	h ^= fnv1_hash(bulbGroupGUID);
	return h;
}

void LifxEmulation::save_state_()
{
	LifxPersistentState state;
	state.yaml_hash = this->yaml_hash_;
	memcpy(state.bulbLabel, bulbLabel, sizeof(bulbLabel));
	memcpy(state.bulbLocation, bulbLocation, sizeof(bulbLocation));
	memcpy(state.bulbLocationGUID, bulbLocationGUID, sizeof(bulbLocationGUID));
	state.bulbLocationTime = bulbLocationTime;
	memcpy(state.bulbGroup, bulbGroup, sizeof(bulbGroup));
	memcpy(state.bulbGroupGUID, bulbGroupGUID, sizeof(bulbGroupGUID));
	state.bulbGroupTime = bulbGroupTime;
	this->pref_.save(&state);
	global_preferences->sync();
	if (debug_) ESP_LOGD(TAG, "Saved state: label=%s, location=%s (%s), group=%s (%s)",
		bulbLabel, bulbLocation, bulbLocationGUID, bulbGroup, bulbGroupGUID);
}

void LifxEmulation::setup()
{
	// Read MAC address from WiFi hardware
	WiFi.macAddress(this->mac);

	// Restore persisted label/location/group if YAML defaults haven't changed
	this->yaml_hash_ = compute_yaml_hash_();
	this->pref_ = global_preferences->make_preference<LifxPersistentState>(fnv1_hash("lifx_emulation_state"));
	LifxPersistentState state;
	if (this->pref_.load(&state) && state.yaml_hash == this->yaml_hash_) {
		memcpy(bulbLabel, state.bulbLabel, sizeof(bulbLabel));
		memcpy(bulbLocation, state.bulbLocation, sizeof(bulbLocation));
		memcpy(bulbLocationGUID, state.bulbLocationGUID, sizeof(bulbLocationGUID));
		bulbLocationTime = state.bulbLocationTime;
		memcpy(bulbGroup, state.bulbGroup, sizeof(bulbGroup));
		memcpy(bulbGroupGUID, state.bulbGroupGUID, sizeof(bulbGroupGUID));
		bulbGroupTime = state.bulbGroupTime;
		ESP_LOGI(TAG, "Restored saved state: label=%s", bulbLabel);
	} else {
		ESP_LOGI(TAG, "Using YAML defaults (no saved state or YAML changed)");
	}

	this->beginUDP();
}

void LifxEmulation::beginUDP()
{
	if (debug_) ESP_LOGD(TAG, "Setting Light Name: %s", bulbLabel);

	// Convert incoming guid strings to byte arrays (strips dashes)
	hexCharacterStringToBytes(bulbGroupGUIDb, (const char *)bulbGroupGUID);
	hexCharacterStringToBytes(bulbLocationGUIDb, (const char *)bulbLocationGUID);

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

void LifxEmulation::incomingUDP(AsyncUDPPacket &packet)
{
	int packetSize = packet.length();
	rx_bytes += packetSize;

	if (packetSize > LIFX_MAX_PACKET_LENGTH) {
		ESP_LOGW(TAG, "Packet too large (%d bytes), ignoring", packetSize);
		return;
	}
	uint8_t packetBuffer[LIFX_MAX_PACKET_LENGTH];
	memcpy(packetBuffer, packet.data(), packetSize);

	IPAddress remote_addr = (packet.remoteIP());
	IPAddress local_addr = packet.localIP();
	int remote_port = packet.remotePort();
	if (debug_) ESP_LOGD(TAG, "Packet Arrived (%s:%d->%s)(%d bytes)",
		remote_addr.toString().c_str(), remote_port,
		local_addr.toString().c_str(), packetSize);

	LifxPacket request;
	processRequest(packetBuffer, packetSize, request);
	handleRequest(request, packet);
}

void LifxEmulation::processRequest(byte *packetBuffer, uint32_t packetSize, LifxPacket &request)
{
	request.size = packetBuffer[0] | (packetBuffer[1] << 8);
	request.protocol = packetBuffer[2] | (packetBuffer[3] << 8);

	memcpy(request.source, packetBuffer + 4, 4);
	memcpy(request.bulbAddress, packetBuffer + 8, 6);

	request.reserved2 = packetBuffer[14] | (packetBuffer[15] << 8);

	memcpy(request.site, packetBuffer + 16, 6);

	request.res_ack = packetBuffer[22];
	request.sequence = packetBuffer[23];
	request.timestamp = (uint64_t)packetBuffer[24] |
						((uint64_t)packetBuffer[25] << 8) |
						((uint64_t)packetBuffer[26] << 16) |
						((uint64_t)packetBuffer[27] << 24) |
						((uint64_t)packetBuffer[28] << 32) |
						((uint64_t)packetBuffer[29] << 40) |
						((uint64_t)packetBuffer[30] << 48) |
						((uint64_t)packetBuffer[31] << 56);
	request.packet_type = packetBuffer[32] | (packetBuffer[33] << 8);
	request.reserved4 = packetBuffer[34] | (packetBuffer[35] << 8);

	uint32_t data_len = packetSize > LifxPacketSize ? packetSize - LifxPacketSize : 0;
	if (data_len > sizeof(request.data))
		data_len = sizeof(request.data);
	memcpy(request.data, packetBuffer + LifxPacketSize, data_len);
	request.data_size = data_len;
}

void LifxEmulation::buildLightStateData(byte *out)
{
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
	memcpy(out, StateData, sizeof(StateData));
}

void LifxEmulation::handleRequest(LifxPacket &request, AsyncUDPPacket &packet)
{
	if (debug_) ESP_LOGD(TAG, "-> %s (0x%02X/%d)", lifx_packet_type_name(request.packet_type), request.packet_type, request.packet_type);

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
		stopWaveform(false);
		hue = word(request.data[2], request.data[1]);
		sat = word(request.data[4], request.data[3]);
		bri = word(request.data[6], request.data[5]);
		kel = word(request.data[8], request.data[7]);
		dur = (uint32_t)request.data[9] << 0 |
			  (uint32_t)request.data[10] << 8 |
			  (uint32_t)request.data[11] << 16 |
			  (uint32_t)request.data[12] << 24;

		setLight();
		if (request.res_ack & RES_REQUIRED)
		{
			response.packet_type = LIGHT_STATUS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			buildLightStateData(response.data);
			response.data_size = 52;
			sendPacket(response, packet);
		}
	}
	break;

	case SET_WAVEFORM:
	{
		// SetWaveform(103) payload:
		// [0] reserved, [1] transient, [2-3] hue, [4-5] sat,
		// [6-7] bri, [8-9] kel, [10-13] period, [14-17] cycles(float),
		// [18-19] skew_ratio(int16), [20] waveform
		trans = request.data[1];
		wave_hue_ = word(request.data[3], request.data[2]);
		wave_sat_ = word(request.data[5], request.data[4]);
		wave_bri_ = word(request.data[7], request.data[6]);
		wave_kel_ = word(request.data[9], request.data[8]);
		period = (uint32_t)request.data[10] |
				 (uint32_t)request.data[11] << 8 |
				 (uint32_t)request.data[12] << 16 |
				 (uint32_t)request.data[13] << 24;
		memcpy(&cycles, &request.data[14], sizeof(float));
		skew_ratio = (int16_t)(request.data[18] | (request.data[19] << 8));
		waveform = request.data[20];

		if (debug_) ESP_LOGD(TAG, "Waveform: type=%u transient=%u period=%u cycles=%.1f skew=%d",
			waveform, trans, period, cycles, skew_ratio);

		startWaveform();

		if (request.res_ack & RES_REQUIRED)
		{
			response.packet_type = LIGHT_STATUS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			buildLightStateData(response.data);
			response.data_size = 52;
			sendPacket(response, packet);
		}
	}
	break;

	case SET_WAVEFORM_OPTIONAL:
	{
		// SetWaveformOptional(119) payload:
		// [0] reserved, [1] transient, [2-3] hue, [4-5] sat,
		// [6-7] bri, [8-9] kel, [10-13] period, [14-17] cycles(float),
		// [18-19] skew_ratio(int16), [20] waveform,
		// [21] set_hue, [22] set_saturation, [23] set_brightness, [24] set_kelvin
		trans = request.data[1];

		// Start from current values, then apply only flagged fields
		wave_hue_ = hue;
		wave_sat_ = sat;
		wave_bri_ = bri;
		wave_kel_ = kel;

		if (request.data[21])
		{
			wave_hue_ = word(request.data[3], request.data[2]);
			if (debug_) ESP_LOGD(TAG, "set hue: %u", wave_hue_);
		}
		if (request.data[22])
		{
			wave_sat_ = word(request.data[5], request.data[4]);
			if (debug_) ESP_LOGD(TAG, "set sat: %u", wave_sat_);
		}
		if (request.data[23])
		{
			wave_bri_ = word(request.data[7], request.data[6]);
			if (debug_) ESP_LOGD(TAG, "set bri: %u", wave_bri_);
		}
		if (request.data[24])
		{
			wave_kel_ = word(request.data[9], request.data[8]);
			if (debug_) ESP_LOGD(TAG, "set kel: %u", wave_kel_);
		}

		period = (uint32_t)request.data[10] |
				 (uint32_t)request.data[11] << 8 |
				 (uint32_t)request.data[12] << 16 |
				 (uint32_t)request.data[13] << 24;
		memcpy(&cycles, &request.data[14], sizeof(float));
		skew_ratio = (int16_t)(request.data[18] | (request.data[19] << 8));
		waveform = request.data[20];

		if (debug_) ESP_LOGD(TAG, "WaveformOptional: type=%u transient=%u period=%u cycles=%.1f skew=%d",
			waveform, trans, period, cycles, skew_ratio);

		startWaveform();

		if (request.res_ack & RES_REQUIRED)
		{
			response.packet_type = LIGHT_STATUS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			buildLightStateData(response.data);
			response.data_size = 52;
			sendPacket(response, packet);
		}
	}
	break;

	case GET_LIGHT_STATE:
	{
		response.res_ack = NO_RESPONSE;
		response.packet_type = LIGHT_STATUS;
		response.protocol = LifxProtocol_AllBulbsResponse;
		buildLightStateData(response.data);
		response.data_size = 52;
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
		stopWaveform(false);
		power_status = word(request.data[1], request.data[0]);
		setLight();
		if (request.res_ack & RES_REQUIRED)
		{
			response.packet_type = (request.packet_type == SET_POWER_STATE) ? POWER_STATE : POWER_STATE2;
			response.protocol = LifxProtocol_AllBulbsResponse;
			byte PowerData[] = {
				lowByte(power_status),
				highByte(power_status)};
			memcpy(response.data, PowerData, sizeof(PowerData));
			response.data_size = sizeof(PowerData);
			sendPacket(response, packet);
		}
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
		memcpy(bulbLabel, request.data, LifxBulbLabelLength);
		save_state_();
		if (request.res_ack & RES_REQUIRED)
		{
			response.packet_type = BULB_LABEL;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, bulbLabel, sizeof(bulbLabel));
			response.data_size = sizeof(bulbLabel);
			sendPacket(response, packet);
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
			memcpy(bulbTags, request.data, LifxBulbTagsLength);
		}

		if (request.packet_type == GET_BULB_TAGS || (request.res_ack & RES_REQUIRED))
		{
			response.packet_type = BULB_TAGS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, bulbTags, sizeof(bulbTags));
			response.data_size = sizeof(bulbTags);
			sendPacket(response, packet);
		}
	}
	break;

	case SET_BULB_TAG_LABELS:
	case GET_BULB_TAG_LABELS:
	{
		if (request.packet_type == SET_BULB_TAG_LABELS)
		{
			memcpy(bulbTagLabels, request.data, LifxBulbTagLabelsLength);
		}

		if (request.packet_type == GET_BULB_TAG_LABELS || (request.res_ack & RES_REQUIRED))
		{
			response.packet_type = BULB_TAG_LABELS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, bulbTagLabels, sizeof(bulbTagLabels));
			response.data_size = sizeof(bulbTagLabels);
			sendPacket(response, packet);
		}
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
		save_state_();
	}
	// fall through to send StateLocation if res_required
	case GET_LOCATION_STATE:
	{
		if (request.packet_type == GET_LOCATION_STATE || (request.res_ack & RES_REQUIRED))
		{
			response.packet_type = LOCATION_STATE;
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
		if (request.packet_type == GET_AUTH_STATE || (request.res_ack & RES_REQUIRED))
		{
			response.packet_type = AUTH_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, authResponse, sizeof(authResponse));
			response.data_size = sizeof(authResponse);
			sendPacket(response, packet);
		}
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
		save_state_();
	}
	// fall through to send StateGroup if res_required
	case GET_GROUP_STATE:
	{
		if (request.packet_type == GET_GROUP_STATE || (request.res_ack & RES_REQUIRED))
		{
			response.packet_type = GROUP_STATE;
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
	}
	break;

	case GET_VERSION_STATE:
	{
		response.packet_type = VERSION_STATE;
		response.protocol = LifxProtocol_AllBulbsResponse;
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
		float rssi = pow(10.0, WiFi.RSSI() / 10.0);
		if (debug_) ESP_LOGD(TAG, "RSSI: %f", rssi);

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
		if (debug_) ESP_LOGD(TAG, "Cloud status changed to: %d", cloudStatus);
	}
	break;

	case GET_CLOUD_STATE:
	{
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
		if (request.packet_type == GET_CLOUD_AUTH || (request.res_ack & RES_REQUIRED))
		{
			response.packet_type = CLOUD_AUTH_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, cloudAuthResponse, sizeof(cloudAuthResponse));
			response.data_size = sizeof(cloudAuthResponse);
			sendPacket(response, packet);
		}
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
		if (request.packet_type == GET_CLOUD_BROKER || (request.res_ack & RES_REQUIRED))
		{
			response.packet_type = CLOUD_BROKER_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, cloudBrokerUrl, sizeof(cloudBrokerUrl));
			response.data_size = sizeof(cloudBrokerUrl);
			sendPacket(response, packet);
		}
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
		if (debug_) ESP_LOGD(TAG, "Ignoring bulb response packet");
	}
	break;

	default:
	{
		ESP_LOGW(TAG, "Unknown packet type: %s (0x%02X/%d)", lifx_packet_type_name(request.packet_type), request.packet_type, request.packet_type);
	}
	break;
	}

	// Handle ack_required (bit 1) - send Acknowledgement(45) independently of res_required
	// Per the LIFX spec, res_required (bit 0) and ack_required (bit 1) are independent flags.
	// res_required is handled by individual message handlers above.
	if (request.res_ack & ACK_REQUIRED)
	{
		if (debug_) ESP_LOGD(TAG, "Acknowledgement Requested");
		response.packet_type = ACKNOWLEDGEMENT;
		response.res_ack = NO_RESPONSE;
		response.protocol = LifxProtocol_AllBulbsResponse;
		memset(response.data, 0, sizeof(response.data));
		response.data_size = 0;
		sendPacket(response, packet);
	}

	// Log non-standard flag bits (observed from real devices, bits 2+ are reserved per spec)
	if (request.res_ack & PAN_REQUIRED)
	{
		if (debug_) ESP_LOGD(TAG, "PAN flag set (0x%02X)", request.res_ack);
	}
}

unsigned int LifxEmulation::sendPacket(LifxPacket &pkt, AsyncUDPPacket &Udpi)
{
	int totalSize = LifxPacketSize + pkt.data_size;

	auto time = this->ha_time_->utcnow();

	// generate a nanosecond epoch timestamp (msec * 1,000,000 + magic)
	uint64_t packetT = (uint64_t)(((uint64_t)time.timestamp * 1000) * 1000000 + LifxMagicNum);
	uint8_t *packetTime = (uint8_t *)&packetT;

	uint8_t _message[LifxPacketSize + sizeof(pkt.data)];
	int _packetLength = 0;

	memset(_message, 0, sizeof(_message));

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

	if (debug_) ESP_LOGD(TAG, "<- %s (0x%02X/%d, %d bytes)", lifx_packet_type_name(pkt.packet_type), pkt.packet_type, pkt.packet_type, _packetLength);
	return _packetLength;
}

void LifxEmulation::startWaveform()
{
	// Save current color as the waveform origin
	orig_hue_ = hue;
	orig_sat_ = sat;
	orig_bri_ = bri;
	orig_kel_ = kel;

	if (period == 0) {
		// Instant: just apply target color directly
		hue = wave_hue_;
		sat = wave_sat_;
		bri = wave_bri_;
		kel = wave_kel_;
		dur = 0;
		setLight();
		return;
	}

	waveform_active_ = true;
	waveform_start_ = millis();
	waveform_last_update_ = 0;

	if (debug_) ESP_LOGD(TAG, "Waveform started: orig(%u,%u,%u,%u) -> target(%u,%u,%u,%u)",
		orig_hue_, orig_sat_, orig_bri_, orig_kel_,
		wave_hue_, wave_sat_, wave_bri_, wave_kel_);
}

void LifxEmulation::stopWaveform(bool restore)
{
	if (!waveform_active_) return;
	waveform_active_ = false;

	if (restore) {
		// Transient: return to original color
		hue = orig_hue_;
		sat = orig_sat_;
		bri = orig_bri_;
		kel = orig_kel_;
	} else {
		// Non-transient: end on target color
		hue = wave_hue_;
		sat = wave_sat_;
		bri = wave_bri_;
		kel = wave_kel_;
	}

	dur = 0;
	setLight();

	if (debug_) ESP_LOGD(TAG, "Waveform stopped (restore=%s)", restore ? "true" : "false");
}

void LifxEmulation::loop()
{
	if (!waveform_active_) return;

	unsigned long now = millis();

	// Rate-limit updates to ~20fps to avoid overwhelming the light hardware
	if (now - waveform_last_update_ < 50) return;
	waveform_last_update_ = now;

	unsigned long elapsed = now - waveform_start_;

	// Check if waveform is complete (cycles > 0 means finite)
	if (cycles > 0 && period > 0) {
		unsigned long total_ms = (unsigned long)(period * cycles);
		if (elapsed >= total_ms) {
			stopWaveform(trans != 0);
			return;
		}
	}

	// Calculate position within current cycle (0.0 to 1.0)
	float cycle_pos = fmodf((float)elapsed / (float)period, 1.0f);

	// Calculate interpolation factor based on waveform type
	// f=0.0 means original color, f=1.0 means target color
	float f = 0.0f;
	switch (waveform) {
	case WAVEFORM_SAW:
		// Linear ramp from original to target, then snap back
		f = cycle_pos;
		break;
	case WAVEFORM_SINE:
		// Smooth sinusoidal oscillation: original -> target -> original
		f = (1.0f - cosf(cycle_pos * 2.0f * (float)M_PI)) / 2.0f;
		break;
	case WAVEFORM_HALF_SINE:
		// Smooth half-sine: original -> target -> original (positive half only)
		f = sinf(cycle_pos * (float)M_PI);
		break;
	case WAVEFORM_TRIANGLE:
		// Linear triangle: original -> target -> original
		f = cycle_pos < 0.5f ? cycle_pos * 2.0f : 2.0f - cycle_pos * 2.0f;
		break;
	case WAVEFORM_PULSE: {
		// Square wave with duty cycle controlled by skew_ratio
		// skew_ratio: -32768..32767 maps to 0..1, duty = 1 - ratio
		float ratio = ((float)skew_ratio + 32768.0f) / 65535.0f;
		f = cycle_pos < (1.0f - ratio) ? 1.0f : 0.0f;
		break;
	}
	default:
		f = 0.0f;
		break;
	}

	// Interpolate HSBK values
	// Hue uses shortest path around the color wheel
	int32_t hue_diff = (int32_t)wave_hue_ - (int32_t)orig_hue_;
	if (hue_diff > 32767) hue_diff -= 65536;
	if (hue_diff < -32768) hue_diff += 65536;
	hue = (uint16_t)((int32_t)orig_hue_ + (int32_t)(f * (float)hue_diff));

	sat = (uint16_t)((float)orig_sat_ + f * ((float)wave_sat_ - (float)orig_sat_));
	bri = (uint16_t)((float)orig_bri_ + f * ((float)wave_bri_ - (float)orig_bri_));
	kel = (uint16_t)((float)orig_kel_ + f * ((float)wave_kel_ - (float)orig_kel_));

	dur = 0; // No transition for waveform frame updates
	setLight();
}

void LifxEmulation::setLight()
{
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

void LifxEmulation::setLightCombined()
{
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
		unsigned long elapsed = millis() - lastChange;
		if (dur > elapsed)
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

void LifxEmulation::setLightDual()
{
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

			unsigned long elapsed = millis() - lastChange;
			if (dur > elapsed)
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

} // namespace lifx_emulation
} // namespace esphome
