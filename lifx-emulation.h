#include "esphome.h"
#include <ESPAsyncUDP.h>

#define DEBUG

#ifdef DEBUG
#define debug_print(x, ...) Serial.print(x, ##__VA_ARGS__)
#define debug_println(x, ...) Serial.println(x, ##__VA_ARGS__)
#else
#define debug_print(x, ...)
#define debug_println(x, ...)
#endif

// this structure doesn't support the response/ack packets properly and needs updated.
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
						 //bool res_required; // response message required
						 //bool ack_required; // acknowledgement message required
						 //bool reservedbits[6]; // 6 bits of reserved?
	uint8_t res_ack;
	uint8_t sequence; //message sequence number

	////protocol header
	uint64_t timestamp;
	uint16_t packet_type; //little endian
	uint16_t reserved4;

	byte data[128];
	int data_size;
};

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
const unsigned int LifxBulbVendor = 1;
const unsigned int LifxBulbProduct = 22;
const unsigned int LifxBulbVersion = 0;
const unsigned int LifxFirmwareVersionMajor = 1;
const unsigned int LifxFirmwareVersionMinor = 5;
const unsigned int LifxMagicNum = 614500;

const byte SERVICE_UDP = 0x01;
const byte SERVICE_TCP = 0x02;
const byte SERVICE_UDP5 = 0x05; // Real bulbs seem to ofer this service too

//incoming packet RES/ACK values
const byte RES_REQUIRED = 0x05;
const byte ACK_REQUIRED = 0x06;
const byte RES_ACK_REQUIRED = 0x00; // unknown value, no real packets seem to set both

// outgoing packet RES/ACK values
const byte NO_RES_NO_ACK = 0x00;
const byte RES_NO_ACK = 0x01; // Real bulb repsonse for STATE_
const byte NO_RES_ACK = 0x02;
const byte RES_ACK = 0x03;

// packet types
const byte GET_PAN_GATEWAY = 0x02;
const byte PAN_GATEWAY = 0x03; // stateService(3)

const byte GET_WIFI_FIRMWARE_STATE = 0x12; // getWifiFirmware(18)
const byte WIFI_FIRMWARE_STATE = 0x13;  // stateWifiFirmware(19)

const byte GET_POWER_STATE = 0x14;
const byte GET_POWER_STATE2 = 0x74;
const byte SET_POWER_STATE = 0x75;
const byte SET_POWER_STATE2 = 0x15;
const byte POWER_STATE = 0x16;
const byte POWER_STATE2 = 0x76;

const byte GET_BULB_LABEL = 0x17;
const byte SET_BULB_LABEL = 0x18;
const byte BULB_LABEL = 0x19;

const byte GET_VERSION_STATE = 0x20; // getVersion(32)
const byte VERSION_STATE = 0x21; // stateVersion(33)

const byte ACKNOWLEDGEMENT = 0x2d;

const byte GET_LOCATION_STATE = 0x30;  // getLocation(48)
const byte SET_LOCATION_STATE = 0x31;
const byte LOCATION_STATE = 0x32; // stateLocation(50)

const byte GET_GROUP_STATE = 0x33;  // getGroup(51)
const byte SET_GROUP_STATE = 0x34;  // setGroup(52)
const byte GROUP_STATE = 0x35; // stateGroup(53) - res_required

const byte GET_AUTH_STATE = 0x36; // Mystery packets queried first by apps, need to add to wireshark plugin
const byte AUTH_STATE = 0x38;

const byte GET_BULB_TAGS = 0x1a;
const byte SET_BULB_TAGS = 0x1b;
const byte BULB_TAGS = 0x1c;

const byte GET_BULB_TAG_LABELS = 0x1d;
const byte SET_BULB_TAG_LABELS = 0x1e;
const byte BULB_TAG_LABELS = 0x1f;

const byte GET_LIGHT_STATE = 0x65; // get(101)
const byte SET_LIGHT_STATE = 0x66; 
const byte LIGHT_STATUS = 0x6b; // state(107) - res_required

const byte SET_WAVEFORM = 0x67;
const byte SET_WAVEFORM_OPTIONAL = 0x77;

const byte GET_MESH_FIRMWARE_STATE = 0x0e;  //getHostFirmware(14)
const byte MESH_FIRMWARE_STATE = 0x0f;

const byte GET_INFARED_STATE = 0x78;
const byte STATE_INFARED_STATE = 0x79;
const byte SET_INFARED_STATE = 0x7A;

// helpers
#define SPACE " "

class lifxUdp : public Component
{
public:
	// no default MAC address, must be set externally (lifxUdp->mac)
	byte mac[6] = {};

	// Bulb name/locationgroup, lame defaults
	char bulbLabel[32] = "B";

	char bulbLocation[32] = "L";
	char bulbLocationGUID[37] = "b49bed4d-77b0-05a3-9ec3-be93d9582f1f";
	uint64_t bulbLocationTime = 1553350342028441856;
									
	char bulbGroup[32] = "G";
	char bulbGroupGUID[37] = "bd93e53d-2014-496f-8cfd-b8886f766d7a";
	uint64_t bulbGroupTime = 1600213602318000000;

    void set_bulbLocation(const char *arg) {strcpy(bulbLocation,arg);}
    void set_bulbLocationGUID(const char *arg) {strcpy(bulbLocationGUID,arg);}
    void set_bulbLocationTime(uint64_t arg) {bulbLocationTime=arg;}

    void set_bulbGroup(const char *arg) {strcpy(bulbGroup,arg);}
    void set_bulbGroupGUID(const char *arg) {strcpy(bulbGroupGUID,arg);}
    void set_bulbGroupTime(uint64_t arg) {bulbGroupTime=arg;}

	/******************************************************************************************************************
	 * beginUDP( char )
	 * This function creates the UDP listener with inline function called for each packet
	******************************************************************************************************************/
	void beginUDP(char bulbLabelToSet[32] = (char *)"Test")
	{
		debug_print(F("Setting Light Name: "));
		strcpy(bulbLabel, bulbLabelToSet);
		debug_println(bulbLabel);

		// start listening for packets
		bool block = 0;
		long pNo = 0;
		if (Udp.listen(LifxPort))
		{
			ESP_LOGD("LIFXUDP", "Listerner Enabled");
			Udp.onPacket(
				[&](AsyncUDPPacket &packet) {
					unsigned long packetTime = millis();
					uint32_t packetSize = packet.length();
					if (packetSize)
					{ //ignore empty packets
						incomingUDP(packet);
					}
					Serial.print(F("Total Packet handling time: "));
					Serial.println(millis() - packetTime);
				});
		}
		//TODO: TCP support necessary?
		//TcpServer.begin();

		setLight(); // will (not) turn light on at boot.... why?
	}

private:
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
	double lastChange = millis();

	byte site_mac[6] = {0x4C, 0x49, 0x46, 0x58, 0x56, 0x32}; // spells out "LIFXV2" - version 2 of the app changes the site address to this...?

	// tags for this bulb, seemingly unused on current real bulbs
	char bulbTags[LifxBulbTagsLength] = {
		0, 0, 0, 0, 0, 0, 0, 0};
	char bulbTagLabels[LifxBulbTagLabelsLength] = "";

	// This is an undocumented packet payload I suspect is a firmware checksum, only checked by official apps
	byte authResponse[56] = {0xd1, 0x74, 0xef, 0x20, 0x68, 0x02, 0x4c, 0x3b, 0x94, 0xf7, 0x24, 0x71, 0x33, 0xc2, 0x98, 0x9a,
							 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							 0x00, 0x39, 0x3b, 0x63, 0x31, 0x64, 0x5b, 0x15};

	// Should probably check if wifi is up first before doing this
	AsyncUDP Udp;

	void setup() override
	{
		// moved to beginUDP()
	}

	void loop() override
	{
		//TODO: Need to add bulb state watching here if things are changed outside this protocol
	}

	/******************************************************************************************************************
	 * incomingUDP( AsyncUDPPacket )
	 * This function is called on all incoming UDP packets
	******************************************************************************************************************/
	void incomingUDP(AsyncUDPPacket &packet)
	{
		int packetSize = packet.length();

		// Changed from a pointer to a memcpy when trying to chase down a bug, likely not needed.  Was worried about simultanious packets.
		//uint8_t *packetBuffer = packet.data();
		uint8_t packetBuffer[packetSize];
		memcpy(packetBuffer, packet.data(), packetSize);

		ESP_LOGD("LIFXUDP", "Packet arrived");
		debug_println();
		debug_print(F("LIFX Packet Arrived ("));
		IPAddress remote_addr(packet.remoteIP());
		int remote_port = packet.remotePort();
		debug_print(remote_addr);
		debug_print(F(":"));
		debug_print(remote_port);
		debug_print(F(")("));

		debug_print(packetSize);
		debug_print(F(" bytes): "));
		for (int i = 0; i < packetSize; i++)
		{
			if (packetBuffer[i] <= 0x0F)
			{
				debug_print(F("0"));
			} // pad with zeros for proper alignment with web packet decoder tools
			debug_print(packetBuffer[i], HEX);
			debug_print(SPACE);
		}
		debug_println();

		LifxPacket request;
		// stuff the packetBuffer into the lifxPacket request struct and return the start of a request object
		processRequest(packetBuffer, packetSize, request);

		//respond to the request.
		handleRequest(request, packet);
	}

	/******************************************************************************************************************
	 * processRequest( byte, float, LifxPacket )
	 * This function is called on all incoming UDP packets to pack a struct
	******************************************************************************************************************/
	void processRequest(byte *packetBuffer, float packetSize, LifxPacket &request)
	{

		request.size = packetBuffer[0] + (packetBuffer[1] << 8);	 //little endian
		request.protocol = packetBuffer[2] + (packetBuffer[3] << 8); //little endian

		// this is the source of the packet
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
			//debug_println(i);
			request.data[j++] = packetBuffer[i];
		}

		request.data_size = j;
	}

	void handleRequest(LifxPacket &request, AsyncUDPPacket &packet)
	{
		debug_print(F("Received packet type 0x"));
		debug_print(request.packet_type, HEX);
		debug_print(F(" ("));
		debug_print(request.packet_type, DEC);
		debug_println(F(")"));

		LifxPacket response;
		for (int x = 0; x < 4; x++)
		{
			response.source[x] = request.source[x];
		}
		// Bulbs must respond with matching number from request
		response.sequence = request.sequence;

		switch (request.packet_type)
		{
			// default to no RES/ACK on packet
			response.res_ack = NO_RES_NO_ACK;

		case GET_PAN_GATEWAY:
		{
			// we are a gateway, so respond to this
			//debug_println(F(">>GET_PAN_GATEWAY Discovery Packet"));

			// respond with the UDP port
			response.packet_type = PAN_GATEWAY;
			response.protocol = LifxProtocol_AllBulbsResponse;
			response.res_ack = RES_NO_ACK;
			byte UDPdata[] = {
				SERVICE_UDP, //UDP
				lowByte(LifxPort),
				highByte(LifxPort),
				0x00,
				0x00};
			byte UDPdata5[] = {
				SERVICE_UDP5, //UDP
				lowByte(LifxPort),
				highByte(LifxPort),
				0x00,
				0x00};

			// A real bulb seems to respond multiple times, once as service type 5
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
			// set the light colors
			hue = word(request.data[2], request.data[1]);
			sat = word(request.data[4], request.data[3]);
			bri = word(request.data[6], request.data[5]);
			kel = word(request.data[8], request.data[7]);
			dur = (uint32_t)request.data[9] << 0 |
				  (uint32_t)request.data[10] << 8 |
				  (uint32_t)request.data[11] << 16 |
				  (uint32_t)request.data[12] << 24;

			setLight();
		}
		break;

		case SET_WAVEFORM:
		{
			hue = word(request.data[3], request.data[2]);
			sat = word(request.data[5], request.data[4]);
			bri = word(request.data[7], request.data[6]);
			kel = word(request.data[9], request.data[8]);
			// duration should be multiplied by cycles in theory but ignored for now
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
			{ // ignore hue?
				debug_print(F(" set hue "));
				hue = word(request.data[3], request.data[2]);
			}
			if (request.data[22])
			{ // ignore sat?
				debug_print(F(" set sat "));
				sat = word(request.data[5], request.data[4]);
			}
			if (request.data[23])
			{ // ignore bri?
				debug_print(F(" set bri "));
				bri = word(request.data[7], request.data[6]);
			}
			if (request.data[24])
			{ // ignore kel?
				debug_print(F(" set kel "));
				kel = word(request.data[9], request.data[8]);
			}
			dur = (uint32_t)request.data[10] << 0 |
				  (uint32_t)request.data[11] << 8 |
				  (uint32_t)request.data[12] << 16 |
				  (uint32_t)request.data[13] << 24;

			// period uint32_t
			// cycles float
			// skew_ratio uint16_t
			// waveform uint8_t

			// not supported, all the waveform flags
			setLight();
		}
		break;

		case GET_LIGHT_STATE:
		{
			response.res_ack = NO_RES_NO_ACK;
			// send the light's state
			response.packet_type = LIGHT_STATUS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			byte StateData[] = {
				lowByte(hue),			//hue
				highByte(hue),			//hue
				lowByte(sat),			//sat
				highByte(sat),			//sat
				lowByte(bri),			//bri
				highByte(bri),			//bri
				lowByte(kel),			//kel
				highByte(kel),			//kel
				lowByte(dim),			// listed as reserved in protocol
				highByte(dim),			// listed as reserved in protocol
				lowByte(power_status),	//power status
				highByte(power_status), //power status
				// label
				lowByte(bulbLabel[0]),
				lowByte(bulbLabel[1]),
				lowByte(bulbLabel[2]),
				lowByte(bulbLabel[3]),
				lowByte(bulbLabel[4]),
				lowByte(bulbLabel[5]),
				lowByte(bulbLabel[6]),
				lowByte(bulbLabel[7]),
				lowByte(bulbLabel[8]),
				lowByte(bulbLabel[9]),
				lowByte(bulbLabel[10]),
				lowByte(bulbLabel[11]),
				lowByte(bulbLabel[12]),
				lowByte(bulbLabel[13]),
				lowByte(bulbLabel[14]),
				lowByte(bulbLabel[15]),
				lowByte(bulbLabel[16]),
				lowByte(bulbLabel[17]),
				lowByte(bulbLabel[18]),
				lowByte(bulbLabel[19]),
				lowByte(bulbLabel[20]),
				lowByte(bulbLabel[21]),
				lowByte(bulbLabel[22]),
				lowByte(bulbLabel[23]),
				lowByte(bulbLabel[24]),
				lowByte(bulbLabel[25]),
				lowByte(bulbLabel[26]),
				lowByte(bulbLabel[27]),
				lowByte(bulbLabel[28]),
				lowByte(bulbLabel[29]),
				lowByte(bulbLabel[30]),
				lowByte(bulbLabel[31]),
				//tags/reserved/unused
				lowByte(bulbTags[0]),
				lowByte(bulbTags[1]),
				lowByte(bulbTags[2]),
				lowByte(bulbTags[3]),
				lowByte(bulbTags[4]),
				lowByte(bulbTags[5]),
				lowByte(bulbTags[6]),
				lowByte(bulbTags[7])};

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

			// real bulbs send an ACK packet on a set power
			// TODO: Refactor to ack properly regardless of incoming packet
			response.packet_type = ACKNOWLEDGEMENT;
			response.protocol = LifxProtocol_AllBulbsResponse;
			byte ack[] = {};
			response.data_size = sizeof(ack);
			memcpy(response.data, ack, response.data_size);
			sendPacket(response, packet);
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
		case GET_BULB_LABEL:
		{
			// set if we are setting
			if (request.packet_type == SET_BULB_LABEL)
			{
				for (int i = 0; i < LifxBulbLabelLength; i++)
				{
					if (bulbLabel[i] != request.data[i])
					{
						bulbLabel[i] = request.data[i];
						//EEPROM.write(EEPROM_BULB_LABEL_START + i, request.data[i]);
					}
				}
			}

			// respond to both get and set commands
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
			// set if we are setting
			if (request.packet_type == SET_BULB_TAGS)
			{
				for (int i = 0; i < LifxBulbTagsLength; i++)
				{
					if (bulbTags[i] != request.data[i])
					{
						bulbTags[i] = lowByte(request.data[i]);
						//EEPROM.write(EEPROM_BULB_TAGS_START + i, request.data[i]);
					}
				}
			}

			// respond to both get and set commands
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
			// set if we are setting
			if (request.packet_type == SET_BULB_TAG_LABELS)
			{
				for (int i = 0; i < LifxBulbTagLabelsLength; i++)
				{
					if (bulbTagLabels[i] != request.data[i])
					{
						bulbTagLabels[i] = request.data[i];
						//EEPROM.write(EEPROM_BULB_TAG_LABELS_START + i, request.data[i]);
					}
				}
			}

			// respond to both get and set commands
			response.packet_type = BULB_TAG_LABELS;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, bulbTagLabels, sizeof(bulbTagLabels));
			response.data_size = sizeof(bulbTagLabels);
			sendPacket(response, packet);
		}
		break;

		case GET_LOCATION_STATE:
		{
			response.packet_type = LOCATION_STATE;
			response.res_ack = RES_NO_ACK;
			response.protocol = LifxProtocol_AllBulbsResponse;
			byte bulbLocationGUIDb[16] = {0};
			hexCharacterStringToBytes( bulbLocationGUIDb, (const char *) bulbLocationGUID );  // strips dashes too
			uint8_t *p = (uint8_t *)&bulbLocationTime; 
			byte LocationStateResponse[56] = {
				bulbLocationGUIDb[3], //Group one big endian (host order LE)
				bulbLocationGUIDb[2],
				bulbLocationGUIDb[1],
				bulbLocationGUIDb[0],
				bulbLocationGUIDb[5], //Group two big endian
				bulbLocationGUIDb[4],
				bulbLocationGUIDb[7], //Group three big endian
				bulbLocationGUIDb[6],
				bulbLocationGUIDb[8], // Group four/five little endian
				bulbLocationGUIDb[9],
				bulbLocationGUIDb[10],
				bulbLocationGUIDb[11],
				bulbLocationGUIDb[12],
				bulbLocationGUIDb[13],
				bulbLocationGUIDb[14],
				bulbLocationGUIDb[15] 
			};
			for( int j = 0; j < sizeof( bulbLocation ); j++ ) {
				LocationStateResponse[j+16] = bulbLocation[j];
			}
			for( int k = 0; k < sizeof(bulbLocationTime); k++ ) {
				LocationStateResponse[k+32+16] = p[k];
			}

			memcpy(response.data, LocationStateResponse, sizeof(LocationStateResponse));
			response.data_size = sizeof(LocationStateResponse);
			sendPacket(response, packet);
		}
		break;

		case GET_AUTH_STATE:
		{
			response.packet_type = AUTH_STATE;
			response.res_ack = RES_NO_ACK;
			response.protocol = LifxProtocol_AllBulbsResponse;
			memcpy(response.data, authResponse, sizeof(authResponse));
			response.data_size = sizeof(authResponse);
			sendPacket(response, packet);
		}
		break;

		case GET_GROUP_STATE:
		{
			response.packet_type = GROUP_STATE;
			response.res_ack = RES_NO_ACK;
			response.protocol = LifxProtocol_AllBulbsResponse;
			byte bulbGroupGUIDb[16] = {0};
			hexCharacterStringToBytes( bulbGroupGUIDb, (const char *) bulbGroupGUID );  // strips dashes too
			uint8_t *p = (uint8_t *)&bulbGroupTime; 
			byte groupStateResponse[56] = {
				bulbGroupGUIDb[3],
				bulbGroupGUIDb[2],
				bulbGroupGUIDb[1],
				bulbGroupGUIDb[0],
				bulbGroupGUIDb[5],
				bulbGroupGUIDb[4],
				bulbGroupGUIDb[7],
				bulbGroupGUIDb[6],
				bulbGroupGUIDb[8],
				bulbGroupGUIDb[9],
				bulbGroupGUIDb[10],
				bulbGroupGUIDb[11],
				bulbGroupGUIDb[12],
				bulbGroupGUIDb[13],
				bulbGroupGUIDb[14],
				bulbGroupGUIDb[15] 
			};
			for( int j = 0; j < sizeof( bulbGroup ); j++ ) {
				groupStateResponse[j+16] = bulbGroup[j];
			}
			for( int k = 0; k < sizeof(bulbGroupTime); k++ ) {
				groupStateResponse[k+32+16] = p[k];
			}

			memcpy(response.data, groupStateResponse, sizeof(groupStateResponse));
			response.data_size = sizeof(groupStateResponse);
			sendPacket(response, packet);
		}
		break;

		case GET_VERSION_STATE:
		{
			// respond to get command
			response.packet_type = VERSION_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			response.res_ack = RES_NO_ACK; // matching real bulb response
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
			// respond to get command
			response.packet_type = MESH_FIRMWARE_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			response.res_ack = RES_NO_ACK;
			// timestamp data comes from observed packet from a LIFX v1.5 bulb
			byte MeshVersionData[] = {
				0x00, 0x94, 0x18, 0x58, 0x1c, 0x05, 0xd9, 0x14, // color 1000 build 1502237570000000000
				0x00, 0x94, 0x18, 0x58, 0x1c, 0x05, 0xd9, 0x14, // color 1000 reserved 0x14d9051c58189400
				0x16, 0x00, 0x01, 0x00							// color 1000 Version 65558
				// 0x00, 0x2e, 0xc3, 0x8b, 0xef, 0x30, 0x86, 0x13, //build timestamp
				// 0xe0, 0x25, 0x76, 0x45, 0x69, 0x81, 0x8b, 0x13, //install timestamp
				// lowByte(LifxFirmwareVersionMinor),
				// highByte(LifxFirmwareVersionMinor),
				// lowByte(LifxFirmwareVersionMajor),
				// highByte(LifxFirmwareVersionMajor)
				//0x00, 0x88, 0x82, 0xaa, 0x7d, 0x15, 0x35, 0x14, // color 1000 build
				//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // color 1000 has no install timestamp
				//0x3e, 0x00, 0x65, 0x00							// color 1000 Version 6619198
			};

			memcpy(response.data, MeshVersionData, sizeof(MeshVersionData));
			response.data_size = sizeof(MeshVersionData);
			sendPacket(response, packet);
		}
		break;

		case GET_WIFI_FIRMWARE_STATE:
		{
			// respond to get command
			response.packet_type = WIFI_FIRMWARE_STATE;
			response.protocol = LifxProtocol_AllBulbsResponse;
			response.res_ack = RES_NO_ACK;
			// timestamp data comes from observed packet from a LIFX v1.5 bulb
			byte WifiVersionData[] = {
				// Original 1000 values
				//0x00, 0xc8, 0x5e, 0x31, 0x99, 0x51, 0x86, 0x13, // Original 1000 (1) bulb build timestamp 1406901652000000000
				//0xc0, 0x0c, 0x07, 0x00, 0x48, 0x46, 0xd9, 0x43, // Original 1000 (1) firmware reserved value 0x43d9464800070cc0
				//0x05, 0x00, 0x01, 0x00							// Original 1000 (1) Version 65541

				// Sometimes the Color 1000 spits out these values instead of the ones below ?!?!
				//0x00, 0x94, 0x18, 0x58, 0x1c, 0x05, 0xd9, 0x14, //build timestamp 1502237570000000000
				//0x00, 0x94, 0x18, 0x58, 0x1c, 0x05, 0xd9, 0x14, //dupe timestamp? 0x14d9051c58189400
				//0x16, 0x00, 0x01, 0x00 // Version 65558

				0x00, 0x88, 0x82, 0xaa, 0x7d, 0x15, 0x35, 0x14, // color 1000 build 1456093684000000000
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // color 1000 has no install timestamp
				0x3e, 0x00, 0x65, 0x00							// color 1000 Version 6619198

				//lowByte(LifxFirmwareVersionMinor),
				//highByte(LifxFirmwareVersionMinor),
				//lowByte(LifxFirmwareVersionMajor),
				//highByte(LifxFirmwareVersionMajor)

			};

			memcpy(response.data, WifiVersionData, sizeof(WifiVersionData));
			response.data_size = sizeof(WifiVersionData);
			sendPacket(response, packet);
		}
		break;

		default:
		{
			debug_print(F("################### Unknown packet type: "));
			debug_print(request.packet_type, HEX);
			debug_print(F("/"));
			debug_println(request.packet_type, DEC);
		}
		break;
		}

		/*
		// should we RES/ACK?
		switch (request.res_ack)
		{
			case NO_RES_NO_ACK:
			{
				// nothing
			}
			break;
			case RES_NO_ACK:
			{
				// Need to figure out if this matters
			}
			break; 
			case NO_RES_ACK:
			{
				break;
				// We are supposed to acknoledge the packet
				response.packet_type = ACKNOWLEDGEMENT;
				response.res_ack = NO_RES_NO_ACK;
				response.protocol = LifxProtocol_AllBulbsResponse;
				response.data = 0;
				response.data_size = 0;
				sendPacket(response, packet);

			}
			break; 

			default:
			{ 
				// unknown packet type
			}

		}
		*/
	}

	unsigned int sendPacket(LifxPacket &pkt, AsyncUDPPacket &Udpi)
	{
		int totalSize = LifxPacketSize + pkt.data_size;

		auto time = ha_time->utcnow();
		// generate a msec value from epoch
		uint64_t packetT = (uint64_t)(((uint64_t)time.timestamp * 1000) * 1000000 + LifxMagicNum);
		// break up large value for packet handling
		uint8_t *packetTime = (uint8_t *)&packetT;

		//debug_println( packetT );

		uint8_t _message[totalSize + 1];
		int _packetLength = 0;

		memset(_message, 0, totalSize + 1); // initialize _message with zeroes

		//// FRAME
		_message[_packetLength++] = (lowByte(totalSize));
		_message[_packetLength++] = (highByte(totalSize));

		// protocol uint16_t
		_message[_packetLength++] = (lowByte(pkt.protocol));
		_message[_packetLength++] = (highByte(pkt.protocol));

		// source/reserved1 .... real bulbs always include this number
		_message[_packetLength++] = pkt.source[0];
		_message[_packetLength++] = pkt.source[1];
		_message[_packetLength++] = pkt.source[2];
		_message[_packetLength++] = pkt.source[3];

		//// FRAME ADDRESS
		// bulbAddress mac address (target)
		for (int i = 0; i < sizeof(mac); i++)
		{
			_message[_packetLength++] = (lowByte(mac[i]));
		}

		// padding MAC
		_message[_packetLength++] = (lowByte(0x00));
		_message[_packetLength++] = (lowByte(0x00));

		// site mac address (LIFXV2)
		for (int i = 0; i < sizeof(site_mac); i++)
		{
			_message[_packetLength++] = (lowByte(site_mac[i]));
		}

		// reserved3: Flags - 6 bits reserved, 1bit ack required, 1bit res required
		_message[_packetLength++] = (lowByte(pkt.res_ack));

		// Sequence.  Real bulbs seem to match the incoming value
		_message[_packetLength++] = (lowByte(pkt.sequence));

		//// PROTOCOL HEADER
		// real bulbs send epoch time in msec plus some strange fixed value (lifxMagicNum?) ...  docs say "reserved"
		_message[_packetLength++] = (lowByte(packetTime[0]));
		_message[_packetLength++] = (lowByte(packetTime[1]));
		_message[_packetLength++] = (lowByte(packetTime[2]));
		_message[_packetLength++] = (lowByte(packetTime[3]));
		_message[_packetLength++] = (lowByte(packetTime[4]));
		_message[_packetLength++] = (lowByte(packetTime[5]));
		_message[_packetLength++] = (lowByte(packetTime[6]));
		_message[_packetLength++] = (lowByte(packetTime[7]));

		//packet type
		_message[_packetLength++] = (lowByte(pkt.packet_type));
		_message[_packetLength++] = (highByte(pkt.packet_type));

		// reserved4
		// This number gets twiddled in the stateService response from a real bulb... sometimes.... other time stays zero
		// 0 0 0 0 0 8 10 12 12 7 6 6 4 0
		_message[_packetLength++] = (lowByte(0x00));
		_message[_packetLength++] = (lowByte(0x00));

		//data
		for (int i = 0; i < pkt.data_size; i++)
		{
			_message[_packetLength++] = (lowByte(pkt.data[i]));
		}

		// async packets get a free reply without specifying target
		Udpi.write(_message, _packetLength);

		// debugging output
		debug_print(F("Sending Packet Type 0x"));
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
			} // pad with zeros for proper alignment
			debug_print(_message[j], HEX);
			debug_print(SPACE);
		}
		//printLifxPacket(pkt, _packetLength);
		debug_println();
		//Serial.println(F("."));
		return _packetLength;
	}

	// this function sets the lights based on values in the globals
	// TODO: refactor to take parameters instead of globals
	void setLight()
	{
		int maxColor = 255;
		int loopDuration = 0;
		double loopRate = millis() - lastChange; // This value will cycle in 49 days from boot up, do we care?
		debug_print(F("Packet rate:"));
		debug_print(loopRate);
		debug_println(F("msec"));
		debug_print(F("Set light - "));
		debug_print(F("hue: "));
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
		debug_print(power_status);
		debug_println(power_status ? " (on)" : "(off)");

		if (power_status && bri)
		{
			float bright = (float)bri / 65535; // Esphome uses 0-1 float for brighness, protocol is 0-65535

			// if we are setting a "white" colour (no saturation)
			if (sat < 1)
			{
				debug_println(F("White light enabled"));
				auto callW = white_led->turn_on();
				auto callC = color_led->turn_off();
				uint16_t mireds = 1000000 / kel; // Esphome requires mireds, protocol is kelvin
				callW.set_color_temperature(mireds);
				callW.set_brightness(bright);
				callW.set_transition_length(dur);
				callC.perform();
				callW.perform();
			}
			else
			{
				uint8_t rgbColor[3];
				auto callW = white_led->turn_off();
				auto callC = color_led->turn_on();

				// Remap to smaller range.... should consider a better hsb2rgb that supports higher precision
				int this_hue = map(hue, 0, 65535, 0, 767);
				int this_sat = map(sat, 0, 65535, 0, 255);
				int this_bri = map(bri, 0, 65535, 0, 255);

				// todo: RGBW(W) math to decide mixing in white light approprately when lowering saturation
				hsb2rgb(this_hue, this_sat, this_bri, rgbColor); // Remap color to RGB from protocol's HSB
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
		{ // shit be off, yo
			auto call = color_led->turn_off();
			call.set_brightness(0);
			call.set_transition_length(dur);
			call.perform();

			auto call2 = white_led->turn_off();
			call2.set_brightness(0);
			// fade to black
			call2.set_transition_length(dur);
			call2.perform();
		}
		lastChange = millis(); // throttle transitions based on last change
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
			if( hexString[charIndex] == '-' ) { 
				//oddLength = !oddLength; // This didn't work.... not sure what happened if we deduct odd number of dashes however
				removed++;
				continue; 
			}

			if (oddLength)
			{
				// If the length is odd
				if (oddCharIndex)
				{
					// odd characters go in high nibble
					currentByte = nibble(hexString[charIndex]) << 4;
				}
				else
				{
					// Even characters go into low nibble
					currentByte |= nibble(hexString[charIndex]);
					byteArray[byteIndex++] = currentByte;
					currentByte = 0;
				}
			}
			else
			{
				// If the length is even
				if (!oddCharIndex)
				{
					// Odd characters go into the high nibble
					currentByte = nibble(hexString[charIndex]) << 4;
				}
				else
				{
					// Odd characters go into low nibble
					currentByte |= nibble(hexString[charIndex]);
					byteArray[byteIndex++] = currentByte;
					currentByte = 0;
				}
			}
		}
	}

	void dumpByteArray(const byte *byteArray, const byte arraySize)
	{

		for (int i = 0; i < arraySize; i++)
		{
			Serial.print("0x");
			if (byteArray[i] < 0x10)
				Serial.print("0");
			Serial.print(byteArray[i], HEX);
			Serial.print(", ");
		}
		Serial.println();
	}

	byte nibble(char c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';

		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;

		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;

		return 0; // Not a valid hexadecimal character
	}

	void str_replace(char *src, char *oldchars, char *newchars)
	{ // utility string function
		char *p = strstr(src, oldchars);
		char buf[37];
		do
		{
			if (p)
			{
				memset(buf, '\0', strlen(buf));
				if (src == p)
				{
					strcpy(buf, newchars);
					strcat(buf, p + strlen(oldchars));
				}
				else
				{
					strncpy(buf, src, strlen(src) - strlen(p));
					strcat(buf, newchars);
					strcat(buf, p + strlen(oldchars));
				}
				memset(src, '\0', strlen(src));
				strcpy(src, buf);
			}
		} while (p && (p = strstr(src, oldchars)));
	}

	void hsi2rgbw(float H, float S, float I, int *rgbw)
	{
		int r, g, b, w;
		float cos_h, cos_1047_h;
		H = fmod(H, 360);				 // cycle H around to 0-360 degrees
		H = 3.14159 * H / (float)180;	 // Convert to radians.
		S = S > 0 ? (S < 1 ? S : 1) : 0; // clamp S and I to interval [0,1]
		I = I > 0 ? (I < 1 ? I : 1) : 0;

		if (H < 2.09439)
		{
			cos_h = cos(H);
			cos_1047_h = cos(1.047196667 - H);
			r = S * 255 * I / 3 * (1 + cos_h / cos_1047_h);
			g = S * 255 * I / 3 * (1 + (1 - cos_h / cos_1047_h));
			b = 0;
			w = 255 * (1 - S) * I;
		}
		else if (H < 4.188787)
		{
			H = H - 2.09439;
			cos_h = cos(H);
			cos_1047_h = cos(1.047196667 - H);
			g = S * 255 * I / 3 * (1 + cos_h / cos_1047_h);
			b = S * 255 * I / 3 * (1 + (1 - cos_h / cos_1047_h));
			r = 0;
			w = 255 * (1 - S) * I;
		}
		else
		{
			H = H - 4.188787;
			cos_h = cos(H);
			cos_1047_h = cos(1.047196667 - H);
			b = S * 255 * I / 3 * (1 + cos_h / cos_1047_h);
			r = S * 255 * I / 3 * (1 + (1 - cos_h / cos_1047_h));
			g = 0;
			w = 255 * (1 - S) * I;
		}

		rgbw[0] = r;
		rgbw[1] = g;
		rgbw[2] = b;
		rgbw[3] = w;
	}
	/******************************************************************************
	 * accepts hue, saturation and brightness values and outputs three 8-bit color
	 * values in an array (color[])
	 *
	 * saturation (sat) and brightness (bright) are 8-bit values.
	 *
	 * hue (index) is a value between 0 and 767. hue values out of range are
	 * rendered as 0.
	 *
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

	uint64_t swap_uint64(uint64_t val)
	{
		val = ((val << 8) & 0xFF00FF00FF00FF00U) | ((val >> 8) & 0x00FF00FF00FF00FFU);
		val = ((val << 16) & 0xFFFF0000FFFF0000U) | ((val >> 16) & 0x0000FFFF0000FFFFU);
		return (val << 32) | (val >> 32);
	}
};
