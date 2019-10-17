#include <EEPROM.h>
#include "esphome.h"
#include <ESPAsyncUDP.h>
#include <ESPAsyncTCP.h>
//#include <ESP8266WebServer.h>
//#include <ESP8266WiFi.h>
#include "./lifx.h"
#include "./color.h"
#define debug_print(x, ...) Serial.print (x, ## __VA_ARGS__)
#define debug_println(x, ...) Serial.println (x, ## __VA_ARGS__)
#define LIFX_HEADER_LENGTH 36
#define LIFX_MAX_PACKET_LENGTH 53

class lifxUdp : public Component {
 public:
  float maxColor = 255;
  // initial bulb values - warm white!
  long power_status = 65535;
  long hue = 0;
  long sat = 0;
  long bri = 65535;
  long kel = 2000;
  long dim = 0;
  // Enter a MAC address and IP address for your device below.
  // The IP address will be dependent on your local network:
  
  // Test ESP hardware
  //4c:11:ae:0d:7f:fe
  //4c:11:ae:0d:1e:5a

  byte mac[6] = { 0x4C, 0x11, 0xAE, 0x0D, 0x1E, 0x5A };
  byte site_mac[6] = { 0x4C, 0x49, 0x46, 0x58, 0x56, 0x32 }; // spells out "LIFXV2" - version 2 of the app changes the site address to this...

  // label (name) for this bulb
  char bulbLabel[LifxBulbLabelLength] = "LED lamp";

  // tags for this bulb
  char bulbTags[LifxBulbTagsLength] = {
	0, 0, 0, 0, 0, 0, 0, 0
  };
  char bulbTagLabels[LifxBulbTagLabelsLength] = "";

  AsyncUDP Udp;
  //WiFiServer TcpServer(LifxPort);
  
  void setup() override {
	
	eepromfake();
  // start listening for packets
	if(Udp.listen(LifxPort)) {
		ESP_LOGD("LIFXUDP", "Listerner Enabled");
		Udp.onPacket(
			[&](AsyncUDPPacket &packet) {
				float packetSize = packet.length();
				if (packetSize) {  //ignore empty packets?  Needed?
					incomingUDP(packet);
				}
			}
		);
	}
	//TODO: TCP support necessary?
	//TcpServer.begin();

  }
  void loop() override {
	  //todo stuff, unneeded for async services
  }
  
  void eepromfake() {
	// not supported properly in ESPHome yet.
	return;
	
	// read in settings from EEPROM (if they exist) for bulb label and tags
	if (EEPROM.read(EEPROM_CONFIG_START) == EEPROM_CONFIG[0]
	  && EEPROM.read(EEPROM_CONFIG_START + 1) == EEPROM_CONFIG[1]
	  && EEPROM.read(EEPROM_CONFIG_START + 2) == EEPROM_CONFIG[2]) {
		debug_println(F("Config exists in EEPROM, reading..."));
		debug_print(F("Bulb label: "));

		for (int i = 0; i < LifxBulbLabelLength; i++) {
		  bulbLabel[i] = EEPROM.read(EEPROM_BULB_LABEL_START + i);
		  debug_print(bulbLabel[i]);
		}

		debug_println();
		debug_print(F("Bulb tags: "));

		for (int i = 0; i < LifxBulbTagsLength; i++) {
		  bulbTags[i] = EEPROM.read(EEPROM_BULB_TAGS_START + i);
		  debug_print(bulbTags[i]);
		}

		debug_println();
		debug_print(F("Bulb tag labels: "));

		for (int i = 0; i < LifxBulbTagLabelsLength; i++) {
		  bulbTagLabels[i] = EEPROM.read(EEPROM_BULB_TAG_LABELS_START + i);
		  debug_print(bulbTagLabels[i]);
	}
	debug_println();
	debug_println(F("Done reading EEPROM config."));
	} else {
		// first time sketch has been run, set defaults into EEPROM
		debug_println(F("Config does not exist in EEPROM, writing..."));

		EEPROM.write(EEPROM_CONFIG_START, EEPROM_CONFIG[0]);
		EEPROM.write(EEPROM_CONFIG_START + 1, EEPROM_CONFIG[1]);
		EEPROM.write(EEPROM_CONFIG_START + 2, EEPROM_CONFIG[2]);

		for (int i = 0; i < LifxBulbLabelLength; i++) {
		  EEPROM.write(EEPROM_BULB_LABEL_START + i, bulbLabel[i]);
		}

		for (int i = 0; i < LifxBulbTagsLength; i++) {
		  EEPROM.write(EEPROM_BULB_TAGS_START + i, bulbTags[i]);
		}

		for (int i = 0; i < LifxBulbTagLabelsLength; i++) {
		  EEPROM.write(EEPROM_BULB_TAG_LABELS_START + i, bulbTagLabels[i]);
		}

		debug_println(F("Done writing EEPROM config."));
	}
	
	debug_println(F("EEPROM dump:"));
	
	for (int i = 0; i < 256; i++) {
		debug_print(EEPROM.read(i));
		debug_print(SPACE);
	}
	
	debug_println();
}
  
  void incomingUDP(AsyncUDPPacket &packet) {
	ESP_LOGD("LIFXUDP", "Packet arrived");
	  
	uint8_t *packetBuffer = packet.data();

	LifxPacket request;
	// stuff the packetBuffer into the lifxPacket request struct.
	processRequest(packetBuffer, sizeof(packetBuffer), request);

	//respond to the request. Now passing the original packet 
	handleRequest(request, packet);
  }
  


  void processRequest(byte *packetBuffer, int packetSize, LifxPacket &request) {

	request.size        = packetBuffer[0] + (packetBuffer[1] << 8); //little endian
	request.protocol    = packetBuffer[2] + (packetBuffer[3] << 8); //little endian
	request.reserved1   = packetBuffer[4] + packetBuffer[5] + packetBuffer[6] + packetBuffer[7];

	byte bulbAddress[] = {
		packetBuffer[8], packetBuffer[9], packetBuffer[10], packetBuffer[11], packetBuffer[12], packetBuffer[13]
	};
	memcpy(request.bulbAddress, bulbAddress, 6);

	request.reserved2   = packetBuffer[14] + packetBuffer[15];

	byte site[] = {
		packetBuffer[16], packetBuffer[17], packetBuffer[18], packetBuffer[19], packetBuffer[20], packetBuffer[21]
	};
	memcpy(request.site, site, 6);

	request.reserved3   = packetBuffer[22] + packetBuffer[23];
	request.timestamp   = packetBuffer[24] + packetBuffer[25] + packetBuffer[26] + packetBuffer[27] +
	  					  packetBuffer[28] + packetBuffer[29] + packetBuffer[30] + packetBuffer[31];
	request.packet_type = packetBuffer[32] + (packetBuffer[33] << 8); //little endian
	request.reserved4   = packetBuffer[34] + packetBuffer[35];

	int i;
	for (i = LifxPacketSize; i < packetSize; i++) {
		request.data[i - LifxPacketSize] = packetBuffer[i];
	}

	request.data_size = i;
  }

void handleRequest(LifxPacket &request, AsyncUDPPacket &packet) {
  debug_print(F("  Received packet type "));
  debug_println(request.packet_type, HEX);

  LifxPacket response;
  switch (request.packet_type) {

	case GET_PAN_GATEWAY:
	  {
		// we are a gateway, so respond to this
		debug_println(F(" GET_PAN_GATEWAY"));

		// respond with the UDP port
		response.packet_type = PAN_GATEWAY;
		response.protocol = LifxProtocol_AllBulbsResponse;
		byte UDPdata[] = {
		  SERVICE_UDP, //UDP
		  lowByte(LifxPort),
		  highByte(LifxPort),
		  0x00,
		  0x00
		};

		memcpy(response.data, UDPdata, sizeof(UDPdata));
		response.data_size = sizeof(UDPdata);
		sendPacket( response, packet );

		// respond with the TCP port details
		response.packet_type = PAN_GATEWAY;
		response.protocol = LifxProtocol_AllBulbsResponse;
		byte TCPdata[] = {
		  SERVICE_TCP, //TCP
		  lowByte(0), // 0 to disable per official docs (should be LifxPort)
		  highByte(0), //same
		  0x00,
		  0x00
		};

		//memcpy(response.data, TCPdata, sizeof(TCPdata));
		//response.data_size = sizeof(TCPdata);
		//sendPacket( response, packet );

	  }
	  break;


	case SET_LIGHT_STATE:
	  {
		// set the light colors
		hue = word(request.data[2], request.data[1]);
		sat = word(request.data[4], request.data[3]);
		bri = word(request.data[6], request.data[5]);
		kel = word(request.data[8], request.data[7]);

		 for(int i=0; i<request.data_size; i++){
		  debug_print(request.data[i], HEX);
		  debug_print(SPACE);
		}
		debug_println();

		setLight();
	  }
	  break;


	case GET_LIGHT_STATE:
	  {
		// send the light's state
		response.packet_type = LIGHT_STATUS;
		response.protocol = LifxProtocol_AllBulbsResponse;
		byte StateData[] = {
		  lowByte(hue),  //hue
		  highByte(hue), //hue
		  lowByte(sat),  //sat
		  highByte(sat), //sat
		  lowByte(bri),  //bri
		  highByte(bri), //bri
		  lowByte(kel),  //kel
		  highByte(kel), //kel
		  lowByte(dim),  //dim
		  highByte(dim), //dim
		  lowByte(power_status),  //power status
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
		  //tags
		  lowByte(bulbTags[0]),
		  lowByte(bulbTags[1]),
		  lowByte(bulbTags[2]),
		  lowByte(bulbTags[3]),
		  lowByte(bulbTags[4]),
		  lowByte(bulbTags[5]),
		  lowByte(bulbTags[6]),
		  lowByte(bulbTags[7])
		};

		memcpy(response.data, StateData, sizeof(StateData));
		response.data_size = sizeof(StateData);
		sendPacket( response, packet );
	  }
	  break;


	case SET_POWER_STATE:
	case SET_POWER_STATE2:
	case GET_POWER_STATE:
	case GET_POWER_STATE2:
	  {
		// set if we are setting
		if ((request.packet_type == SET_POWER_STATE) | (request.packet_type == SET_POWER_STATE2)) {
		  power_status = word(request.data[1], request.data[0]);
		  setLight();
		}

		// respond to both get and set commands
		response.packet_type = POWER_STATE;
		response.protocol = LifxProtocol_AllBulbsResponse;
		byte PowerData[] = {
		  lowByte(power_status),
		  highByte(power_status)
		};

		memcpy(response.data, PowerData, sizeof(PowerData));
		response.data_size = sizeof(PowerData);
		sendPacket( response, packet );
	  }
	  break;


	case SET_BULB_LABEL:
	case GET_BULB_LABEL:
	  {
		// set if we are setting
		if (request.packet_type == SET_BULB_LABEL) {
		  for (int i = 0; i < LifxBulbLabelLength; i++) {
			if (bulbLabel[i] != request.data[i]) {
			  bulbLabel[i] = request.data[i];
			  EEPROM.write(EEPROM_BULB_LABEL_START + i, request.data[i]);
			}
		  }
		}

		// respond to both get and set commands
		response.packet_type = BULB_LABEL;
		response.protocol = LifxProtocol_AllBulbsResponse;
		memcpy(response.data, bulbLabel, sizeof(bulbLabel));
		response.data_size = sizeof(bulbLabel);
		sendPacket( response, packet );
	  }
	  break;


	case SET_BULB_TAGS:
	case GET_BULB_TAGS:
	  {
		// set if we are setting
		if (request.packet_type == SET_BULB_TAGS) {
		  for (int i = 0; i < LifxBulbTagsLength; i++) {
			if (bulbTags[i] != request.data[i]) {
			  bulbTags[i] = lowByte(request.data[i]);
			  EEPROM.write(EEPROM_BULB_TAGS_START + i, request.data[i]);
			}
		  }
		}

		// respond to both get and set commands
		response.packet_type = BULB_TAGS;
		response.protocol = LifxProtocol_AllBulbsResponse;
		memcpy(response.data, bulbTags, sizeof(bulbTags));
		response.data_size = sizeof(bulbTags);
		sendPacket( response, packet );
	  }
	  break;


	case SET_BULB_TAG_LABELS:
	case GET_BULB_TAG_LABELS:
	  {
		// set if we are setting
		if (request.packet_type == SET_BULB_TAG_LABELS) {
		  for (int i = 0; i < LifxBulbTagLabelsLength; i++) {
			if (bulbTagLabels[i] != request.data[i]) {
			  bulbTagLabels[i] = request.data[i];
			  EEPROM.write(EEPROM_BULB_TAG_LABELS_START + i, request.data[i]);
			}
		  }
		}

		// respond to both get and set commands
		response.packet_type = BULB_TAG_LABELS;
		response.protocol = LifxProtocol_AllBulbsResponse;
		memcpy(response.data, bulbTagLabels, sizeof(bulbTagLabels));
		response.data_size = sizeof(bulbTagLabels);
		sendPacket( response, packet );
	  }
	  break;


	case GET_VERSION_STATE:
	  {
		// respond to get command
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
		  0x00
		};

		memcpy(response.data, VersionData, sizeof(VersionData));
		response.data_size = sizeof(VersionData);
		sendPacket( response, packet );

		/*
		// respond again to get command (real bulbs respond twice, slightly diff data (see below)
		response.packet_type = VERSION_STATE;
		response.protocol = LifxProtocol_AllBulbsResponse;
		byte VersionData2[] = {
		  lowByte(LifxVersionVendor), //vendor stays the same
		  highByte(LifxVersionVendor),
		  0x00,
		  0x00,
		  lowByte(LifxVersionProduct*2), //product is 2, rather than 1
		  highByte(LifxVersionProduct*2),
		  0x00,
		  0x00,
		  0x00, //version is 0, rather than 1
		  0x00,
		  0x00,
		  0x00
		  };

		memcpy(response.data, VersionData2, sizeof(VersionData2));
		response.data_size = sizeof(VersionData2);
		sendPacket( response, packet );
		*/
	  }
	  break;


	case GET_MESH_FIRMWARE_STATE:
	  {
		// respond to get command
		response.packet_type = MESH_FIRMWARE_STATE;
		response.protocol = LifxProtocol_AllBulbsResponse;
		// timestamp data comes from observed packet from a LIFX v1.5 bulb
		byte MeshVersionData[] = {
		  0x00, 0x2e, 0xc3, 0x8b, 0xef, 0x30, 0x86, 0x13, //build timestamp
		  0xe0, 0x25, 0x76, 0x45, 0x69, 0x81, 0x8b, 0x13, //install timestamp
		  lowByte(LifxFirmwareVersionMinor),
		  highByte(LifxFirmwareVersionMinor),
		  lowByte(LifxFirmwareVersionMajor),
		  highByte(LifxFirmwareVersionMajor)
		};

		memcpy(response.data, MeshVersionData, sizeof(MeshVersionData));
		response.data_size = sizeof(MeshVersionData);
		sendPacket( response, packet );
	  }
	  break;


	case GET_WIFI_FIRMWARE_STATE:
	  {
		// respond to get command
		response.packet_type = WIFI_FIRMWARE_STATE;
		response.protocol = LifxProtocol_AllBulbsResponse;
		// timestamp data comes from observed packet from a LIFX v1.5 bulb
		byte WifiVersionData[] = {
		  0x00, 0xc8, 0x5e, 0x31, 0x99, 0x51, 0x86, 0x13, //build timestamp
		  0xc0, 0x0c, 0x07, 0x00, 0x48, 0x46, 0xd9, 0x43, //install timestamp
		  lowByte(LifxFirmwareVersionMinor),
		  highByte(LifxFirmwareVersionMinor),
		  lowByte(LifxFirmwareVersionMajor),
		  highByte(LifxFirmwareVersionMajor)
		};

		memcpy(response.data, WifiVersionData, sizeof(WifiVersionData));
		response.data_size = sizeof(WifiVersionData);
		sendPacket( response, packet );
	  }
	  break;


	default:
	  {
		  debug_print(F("Unknown packet type: "));
		  debug_println(request.packet_type, DEC);
	  }
	  break;
  }
}

void sendPacket(LifxPacket &pkt, AsyncUDPPacket &packet) {
  sendUDPPacket(pkt, packet);
  // todo re-add tcp here if needed
}

unsigned int sendUDPPacket(LifxPacket &pkt, AsyncUDPPacket &Udpi) {
  // broadcast packet on local subnet
  IPAddress remote_addr(Udpi.remoteIP());
  IPAddress broadcast_addr(remote_addr[0], remote_addr[1], remote_addr[2], 255);
  int remote_port = Udpi.remotePort();
  debug_println(F("+UDP sending: "));
  debug_print(remote_addr);
  debug_print(F(":"));
  debug_print(remote_port);
  debug_println();
  printLifxPacket(pkt);
  debug_println();
  
  uint8_t _message[LIFX_MAX_PACKET_LENGTH];
  int _packetLength;
  uint8_t _sequenceNum;
 
  memset(_message, 0, LIFX_MAX_PACKET_LENGTH);   // initialize _message with zeroes
  //_packetLength = LIFX_HEADER_LENGTH;
  _sequenceNum = 0;

    // size
  _message[_packetLength++] = (lowByte(LifxPacketSize + pkt.data_size));
  _message[_packetLength++] = (highByte(LifxPacketSize + pkt.data_size));

  // protocol
  _message[_packetLength++] = (lowByte(pkt.protocol));
  _message[_packetLength++] = (highByte(pkt.protocol));

  // reserved1
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));

  // bulbAddress mac address
  for (int i = 0; i < sizeof(mac); i++) {
	_message[_packetLength++] = (lowByte(mac[i]));
  }

  // reserved2
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));

  // site mac address
  for (int i = 0; i < sizeof(site_mac); i++) {
	_message[_packetLength++] = (lowByte(site_mac[i]));
  }

  // reserved3
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));

  // timestamp
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));

  //packet type
  _message[_packetLength++] = (lowByte(pkt.packet_type));
  _message[_packetLength++] = (highByte(pkt.packet_type));

  // reserved4
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));

  //data
  for (int i = 0; i < pkt.data_size; i++) {
	_message[_packetLength++] = (lowByte(pkt.data[i]));
  }

  Udpi.write(_message, _packetLength);
  return LifxPacketSize + pkt.data_size;
}

unsigned int sendTCPPacket(LifxPacket &pkt) {

  ESP_LOGD("LIFXUDP", "+TCP ");
  printLifxPacket(pkt);
  ESP_LOGD("LIFXUDP", "");
  
  byte TCPBuffer[128]; //buffer to hold outgoing packet,
  int byteCount = 0;

  // size
  TCPBuffer[byteCount++] = lowByte(LifxPacketSize + pkt.data_size);
  TCPBuffer[byteCount++] = highByte(LifxPacketSize + pkt.data_size);

  // protocol
  TCPBuffer[byteCount++] = lowByte(pkt.protocol);
  TCPBuffer[byteCount++] = highByte(pkt.protocol);

  // reserved1
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // bulbAddress mac address
  for (int i = 0; i < sizeof(mac); i++) {
	TCPBuffer[byteCount++] = lowByte(mac[i]);
  }

  // reserved2
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // site mac address
  for (int i = 0; i < sizeof(site_mac); i++) {
	TCPBuffer[byteCount++] = lowByte(site_mac[i]);
  }

  // reserved3
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // timestamp
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  //packet type
  TCPBuffer[byteCount++] = lowByte(pkt.packet_type);
  TCPBuffer[byteCount++] = highByte(pkt.packet_type);

  // reserved4
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  //data
  for (int i = 0; i < pkt.data_size; i++) {
	TCPBuffer[byteCount++] = lowByte(pkt.data[i]);
  }

  //client.write(TCPBuffer, byteCount);

  return LifxPacketSize + pkt.data_size;
}

// print out a LifxPacket data structure as a series of hex bytes - used for DEBUG
void printLifxPacket(LifxPacket &pkt) {
  // size
  debug_print(lowByte(LifxPacketSize + pkt.data_size), HEX);
  debug_print(SPACE);
  debug_print(highByte(LifxPacketSize + pkt.data_size), HEX);
  debug_print(SPACE);

  // protocol
  debug_print(lowByte(pkt.protocol), HEX);
  debug_print(SPACE);
  debug_print(highByte(pkt.protocol), HEX);
  debug_print(SPACE);

  // reserved1
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  // bulbAddress mac address
  for (int i = 0; i < sizeof(mac); i++) {
	debug_print(lowByte(mac[i]), HEX);
	debug_print(SPACE);
  }

  // reserved2
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  // site mac address
  for (int i = 0; i < sizeof(site_mac); i++) {
	debug_print(lowByte(site_mac[i]), HEX);
	debug_print(SPACE);
  }

  // reserved3
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  // timestamp
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  //packet type
  debug_print(lowByte(pkt.packet_type), HEX);
  debug_print(SPACE);
  debug_print(highByte(pkt.packet_type), HEX);
  debug_print(SPACE);

  // reserved4
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  //data
  for (int i = 0; i < pkt.data_size; i++) {
	debug_print(pkt.data[i], HEX);
	debug_print(SPACE);
  }
}

void setLight() {
  debug_print(F("Set light - "));
  debug_print(F("hue: "));
  debug_print(hue);
  debug_print(F(", sat: "));
  debug_print(sat);
  debug_print(F(", bri: "));
  debug_print(bri);
  debug_print(F(", kel: "));
  debug_print(kel);
  debug_print(F(", power: "));
  debug_print(power_status);
  debug_println(power_status ? " (on)" : "(off)");
  auto call = lifxtest->turn_on();

  if (power_status) {
	int this_hue = map(hue, 0, 65535, 0, 767);
	int this_sat = map(sat, 0, 65535, 0, 255);
	int this_bri = map(bri, 0, 65535, 0, 255);
	
	// if we are setting a "white" colour (kelvin temp)
	if (kel > 0 && this_sat < 1) {
	  // convert kelvin to RGB
	  rgb kelvin_rgb;
	  kelvin_rgb = kelvinToRGB(kel);

	  // convert the RGB into HSV
	  hsv kelvin_hsv;
	  kelvin_hsv = rgb2hsv(kelvin_rgb);

	  // set the new values ready to go to the bulb (brightness does not change, just hue and saturation)
	  this_hue = map(kelvin_hsv.h, 0, 359, 0, 767);
	  this_sat = map(kelvin_hsv.s * 1000, 0, 1000, 0, 255); //multiply the sat by 1000 so we can map the percentage value returned by rgb2hsv
	}

	uint8_t rgbColor[3];
	hsb2rgb(this_hue, this_sat, this_bri, rgbColor);

	uint8_t r = map(rgbColor[0], 0, 255, 0, 32);
	uint8_t g = map(rgbColor[1], 0, 255, 0, 32);
	uint8_t b = map(rgbColor[2], 0, 255, 0, 32);
	
	// LIFXBulb.fadeHSB(this_hue, this_sat, this_bri);
	////led_strip.setPixelColor (0, r, g, b);
	call.set_rgb(r,g,b);
	call.set_brightness(1);
	call.set_transition_length(0);

  }
  else {
	call = lifxtest->turn_off();
	call.set_rgb(0,0,0);
	call.set_brightness(0);
	call.set_transition_length(0);

	// LIFXBulb.fadeHSB(0, 0, 0);
	////led_strip.setPixelColor (0, 0, 0, 0);
  }
  call.perform();
 ////led_strip.show ();
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

  else if ( index < 768)
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

  color[0]  = (uint8_t)r_temp;
  color[1]  = (uint8_t)g_temp;
  color[2] = (uint8_t)b_temp;
}

};