#include "esphome.h"
#include <ESPAsyncUDP.h>

// if you turn off debug printing the whole thing crashes like crazy...
#define DEBUG

#ifdef DEBUG
 #define debug_print(x, ...) Serial.print (x, ## __VA_ARGS__)
 #define debug_println(x, ...) Serial.println (x, ## __VA_ARGS__)
#else
 #define debug_print(x, ...)
 #define debug_print(x, ...) 
 #define debug_println(x, ...)
#endif

/* Source: http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both */

struct rgbb {
    double r;       // percent
    double g;       // percent
    double b;       // percent
};

struct hsv {
    double h;       // angle in degrees
    double s;       // percent
    double v;       // percent
};

static hsv      rgb2hsv(rgbb in);
static rgbb      hsv2rgb(hsv in);

hsv rgb2hsv(rgbb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if( max > 0.0 ) {
        out.s = (delta / max);                  // s
    } else {
        // r = g = b = 0                        // s = 0, v is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


rgbb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgbb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}

/* Convert Kelvin to RGB: based on http://bit.ly/1bc83he */

rgbb kelvinToRGB(long kelvin) {
  rgbb kelvin_rgb;
  long temperature = kelvin / 100;

  if(temperature <= 66) {
    kelvin_rgb.r = 255;
  } 
  else {
    kelvin_rgb.r = temperature - 60;
    kelvin_rgb.r = 329.698727446 * pow(kelvin_rgb.r, -0.1332047592);
    if(kelvin_rgb.r < 0) kelvin_rgb.r = 0;
    if(kelvin_rgb.r > 255) kelvin_rgb.r = 255;
  }

  if(temperature <= 66) {
    kelvin_rgb.g = temperature;
    kelvin_rgb.g = 99.4708025861 * log(kelvin_rgb.g) - 161.1195681661;
    if(kelvin_rgb.g < 0) kelvin_rgb.g = 0;
    if(kelvin_rgb.g > 255) kelvin_rgb.g = 255;
  } 
  else {
    kelvin_rgb.g = temperature - 60;
    kelvin_rgb.g = 288.1221695283 * pow(kelvin_rgb.g, -0.0755148492);
    if(kelvin_rgb.g < 0) kelvin_rgb.g = 0;
    if(kelvin_rgb.g > 255) kelvin_rgb.g = 255;
  }

  if(temperature >= 66) {
    kelvin_rgb.b = 255;
  } 
  else {
    if(temperature <= 19) {
      kelvin_rgb.b = 0;
    } 
    else {
      kelvin_rgb.b = temperature - 10;
      kelvin_rgb.b = 138.5177312231 * log(kelvin_rgb.b) - 305.0447927307;
      if(kelvin_rgb.b < 0) kelvin_rgb.b = 0;
      if(kelvin_rgb.b > 255) kelvin_rgb.b = 255;
    }
  }

  return kelvin_rgb;
}

struct LifxPacket {
  ////frame
  uint16_t size; //little endian
  uint16_t protocol; //little endian
  byte source[4];  //source

  //frame address
  byte bulbAddress[6];  //device mac
  uint16_t reserved2; // mac padding
  byte site[6];  // "reserved"
   //bool res_required; // response message required
   //bool ack_required; // acknowledgement message required
   //bool reservedbits[6]; // 6 bits of reserved?
  uint8_t reserved3;
  uint8_t sequence; //message sequence number

  ////protocol header
  uint64_t timestamp;
  uint16_t packet_type; //little endian
  uint16_t reserved4;
  
  byte data[128];
  int data_size;
};

const unsigned int LifxProtocol_AllBulbsResponse = 21504; // 0x5400
const unsigned int LifxProtocol_AllBulbsRequest  = 13312; // 0x3400
const unsigned int LifxProtocol_BulbCommand      = 5120;  // 0x1400

const unsigned int LifxPacketSize      		= 36;
const unsigned int LifxPort            		= 56700;  // local port to listen on
const unsigned int LifxBulbLabelLength 		= 32;
const unsigned int LifxBulbTagsLength  		= 8;
const unsigned int LifxBulbTagLabelsLength 	= 32;
const unsigned int LIFX_MAX_PACKET_LENGTH 	= 53;

// firmware versions, etc
const unsigned int LifxBulbVendor  = 1;
const unsigned int LifxBulbProduct = 1;
const unsigned int LifxBulbVersion = 512;
const unsigned int LifxFirmwareVersionMajor = 1;
const unsigned int LifxFirmwareVersionMinor = 5;

const byte SERVICE_UDP = 0x01;
const byte SERVICE_TCP = 0x02;

// packet types
const byte GET_PAN_GATEWAY = 0x02;
const byte PAN_GATEWAY = 0x03;

const byte GET_WIFI_FIRMWARE_STATE = 0x12;
const byte WIFI_FIRMWARE_STATE = 0x13;

const byte GET_POWER_STATE = 0x14;
const byte GET_POWER_STATE2 = 0x74;
const byte SET_POWER_STATE = 0x75;
const byte SET_POWER_STATE2 = 0x15;
const byte POWER_STATE = 0x16;
const byte POWER_STATE2 = 0x76;

const byte GET_BULB_LABEL = 0x17;
const byte SET_BULB_LABEL = 0x18;
const byte BULB_LABEL = 0x19;

const byte GET_VERSION_STATE = 0x20;
const byte VERSION_STATE = 0x21;

const byte GET_LOCATION_STATE = 0x30;
const byte LOCATION_STATE = 0x50;

const byte GET_GROUP_STATE = 0x33;
const byte GROUP_STATE = 0x25;

const byte GET_BULB_TAGS = 0x1a;
const byte SET_BULB_TAGS = 0x1b;
const byte BULB_TAGS = 0x1c;

const byte GET_BULB_TAG_LABELS = 0x1d;
const byte SET_BULB_TAG_LABELS = 0x1e;
const byte BULB_TAG_LABELS = 0x1f;

const byte GET_LIGHT_STATE = 0x65;
const byte SET_LIGHT_STATE = 0x66;
const byte LIGHT_STATUS = 0x6b;

const byte SET_WAVEFORM = 0x67;
const byte SET_WAVEFORM_OPTIONAL = 0x77;

const byte GET_MESH_FIRMWARE_STATE = 0x0e;
const byte MESH_FIRMWARE_STATE = 0x0f;

// unused eeprom defines, not working under esphome yet
#define EEPROM_BULB_LABEL_START 0 // 32 bytes long
#define EEPROM_BULB_TAGS_START 32 // 8 bytes long
#define EEPROM_BULB_TAG_LABELS_START 40 // 32 bytes long
// future data for EEPROM will start at 72...

#define EEPROM_CONFIG "AL1" // 3 byte identifier for this sketch's EEPROM settings
#define EEPROM_CONFIG_START 253 // store EEPROM_CONFIG at the end of EEPROM

// helpers
#define SPACE " "

class lifxUdp: public Component {
 public:
  float maxColor = 255;
  // initial bulb values - warm white!
  uint16_t power_status = 65535;
  uint16_t hue = 0;
  uint16_t sat = 0;
  uint16_t bri = 65535;
  uint16_t kel = 2000;
  byte trans = 0;
  uint32_t period = 0;
  float cycles = 0;
  int skew_ratio = 0; //signed 16 bit int
  uint8_t waveform = 0;
  uint8_t set_hue = 1;
  uint8_t set_sat = 1;
  uint8_t set_bri = 1;
  uint8_t set_kel = 1;
  bool UPDATING = 0;


  long dim = 0;

  uint32_t dur = 0;
  uint8_t _sequence = 0x00;

  byte mac[6];
  byte site_mac[6] = { 0x4C, 0x49, 0x46, 0x58, 0x56, 0x32 }; // spells out "LIFXV2" - version 2 of the app changes the site address to this...

  // label (name) for this bulb
  char bulbLabel[LifxBulbLabelLength]; 

  // tags for this bulb
  char bulbTags[LifxBulbTagsLength] = {
	0, 0, 0, 0, 0, 0, 0, 0
  };
  char bulbTagLabels[LifxBulbTagLabelsLength] = "";

  AsyncUDP Udp;

  void setup() override {
	  // moved to beginUDP()
  }

  void beginUDP( byte bulbMac[6], char lifxLightName[LifxBulbLabelLength] ) {
	// real bulbs all have a mac starting with D0:73:D5:
	for(int i=0; i<6; i++){
		mac[i] = bulbMac[i];
	}
	debug_print(F("Setting Light Name: "));
	debug_println(lifxLightName);
	for(int j=0; j<LifxBulbLabelLength; j++){
		bulbLabel[j] = lifxLightName[j];
	}
	debug_println(bulbLabel);

	eepromfake();
    // start listening for packets
	if(Udp.listen(LifxPort)) {
		ESP_LOGD("LIFXUDP", "Listerner Enabled");
		Udp.onPacket(
			[&](AsyncUDPPacket &packet) {
				float packetSize = packet.length();
				if (packetSize) {  //ignore empty packets?  Needed?
					incomingUDP(packet, packetSize);
				}
			}
		);
	}
	//TODO: TCP support necessary?
	//TcpServer.begin();
	setLight();
  }
  void loop() override {
	  //todo stuff, unneeded for async services
  }
  
  void eepromfake() {
	// not supported properly in ESPHome yet.
	return;
	/*
	debug_println(F("EEPROM dump:"));
	
	for (int i = 0; i < 256; i++) {
		debug_print(EEPROM.read(i));
		debug_print(SPACE);
	}
	
	debug_println();
	
	
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
	*/
}
  
  void incomingUDP(AsyncUDPPacket &packet, float packetSize) {
	uint8_t *packetBuffer = packet.data();

	ESP_LOGD("LIFXUDP", "Packet arrived");
	debug_println();
	debug_print(F("LIFX Packet Arrived ("));
	debug_print(packetSize);
	debug_print(F("): "));
	for(int i=0; i<packetSize; i++){
		if(packetBuffer[i] <= 0x0F) { debug_print(F("0"));}  // pad with zeros for proper alignment
		debug_print(packetBuffer[i], HEX);
		debug_print(SPACE);
	}
	debug_println();

	LifxPacket request;
	// stuff the packetBuffer into the lifxPacket request struct.
	processRequest(packetBuffer, packetSize, request);

	//respond to the request. Now passing the original packet for easier repsonse to packet
	handleRequest(request, packet);
  }
  


  void processRequest(byte *packetBuffer, float packetSize, LifxPacket &request) {

	request.size        = packetBuffer[0] + (packetBuffer[1] << 8); //little endian
	request.protocol    = packetBuffer[2] + (packetBuffer[3] << 8); //little endian

	// this is the source of the packet
	byte sourceID[] = { packetBuffer[4], packetBuffer[5], packetBuffer[6], packetBuffer[7] };
	memcpy(request.source, sourceID, 4);

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
	request.sequence	= _sequence++;
	request.timestamp   = packetBuffer[24] + packetBuffer[25] + packetBuffer[26] + packetBuffer[27] +
	  					  packetBuffer[28] + packetBuffer[29] + packetBuffer[30] + packetBuffer[31];
	request.packet_type = packetBuffer[32] + (packetBuffer[33] << 8); //little endian
	request.reserved4   = packetBuffer[34] + packetBuffer[35];

	int j=0;
	for (unsigned int i = LifxPacketSize; i < packetSize; i++) {
		//debug_println(i);
		request.data[j++] = packetBuffer[i];
	}

	request.data_size = j;
  }

void handleRequest(LifxPacket &request, AsyncUDPPacket &packet) {
  debug_print(F("Received packet type "));
  debug_println(request.packet_type, HEX);
  LifxPacket response;
  for( int x = 0; x < sizeof( request.source ); x++ ) {
	  	response.source[x] = request.source[x];
		//debug_print(response.source[x], HEX);
  }
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

		/*
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
		*/
	  }
	  break;


	case SET_LIGHT_STATE:
	  {
		// set the light colors
		hue = word(request.data[2], request.data[1]);
		sat = word(request.data[4], request.data[3]);
		bri = word(request.data[6], request.data[5]);
		kel = word(request.data[8], request.data[7]);
		byte dur[] = {request.data[12], request.data[11], request.data[10], request.data[9] };

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
		byte dur[] = {request.data[13], request.data[12], request.data[11], request.data[10] };
		setLight();
	}
	break;
	
	case SET_WAVEFORM_OPTIONAL:
	  {
		if( request.data[21] ) { // ignore hue?
			debug_print(F(" set hue "));
			hue = word(request.data[3], request.data[2]);
		}
		if( request.data[22] ) { // ignore sat?
			debug_print(F(" set sat "));
			sat = word(request.data[5], request.data[4]);
		}		
		if( request.data[23] ) { // ignore bri?
			debug_print(F(" set bri "));
			bri = word(request.data[7], request.data[6]);
		}
		if( request.data[24] ) { // ignore kel?
			debug_print(F(" set kel "));
			kel = word(request.data[9], request.data[8]);
		}
		byte dur[] = {request.data[13], request.data[12], request.data[11], request.data[10] };

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
		// send the light's state
		response.packet_type = LIGHT_STATUS;
		response.protocol = LifxProtocol_BulbCommand;  // home assistnat crashes if you use all bulb response here
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
			  //EEPROM.write(EEPROM_BULB_LABEL_START + i, request.data[i]);
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
			  //EEPROM.write(EEPROM_BULB_TAGS_START + i, request.data[i]);
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
			  //EEPROM.write(EEPROM_BULB_TAG_LABELS_START + i, request.data[i]);
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

	case GET_LOCATION_STATE:
	  {
	  response.packet_type = LOCATION_STATE;
	  response.protocol = LifxProtocol_AllBulbsResponse;
	  break;
	  }
	case GET_GROUP_STATE:
	{
	  response.packet_type = GROUP_STATE;
	  response.protocol = LifxProtocol_AllBulbsResponse;
	  break;
	}
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
		  lowByte(LifxBulbVendor), //vendor stays the same
		  highByte(LifxBulbVendor),
		  0x00,
		  0x00,
		  lowByte(LifxBulbProduct*2), //product is 2, rather than 1
		  highByte(LifxBulbProduct*2),
		  0x00,
		  0x00,
		  0x00, //version is 0, rather than 1
		  0x00,
		  0x00,
		  0x00
		  };

		memcpy(response.data, VersionData2, sizeof(VersionData2));
		response.data_size = sizeof(VersionData2);
		*/
		//sendPacket( response, packet );
		
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
  int remote_port = Udpi.remotePort();
  debug_print(F("+UDP packet building triggered by: "));
  debug_print(remote_addr);
  debug_print(F(":"));
  debug_print(remote_port);
  debug_println();
  
  uint8_t _message[LIFX_MAX_PACKET_LENGTH];
  int _packetLength = 0;
 
  memset(_message, 0, LIFX_MAX_PACKET_LENGTH);   // initialize _message with zeroes

  //// FRAME 
  _message[_packetLength++] = (lowByte(LifxPacketSize + pkt.data_size));
  _message[_packetLength++] = (highByte(LifxPacketSize + pkt.data_size));

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
  for (int i = 0; i < sizeof(mac); i++) {
	_message[_packetLength++] = (lowByte(mac[i]));
  }

  // padding MAC
  _message[_packetLength++] = (lowByte(0x00));
  _message[_packetLength++] = (lowByte(0x00));

  // site mac address (LIFXV2)
  for (int i = 0; i < sizeof(site_mac); i++) {
	_message[_packetLength++] = (lowByte(site_mac[i]));
  }

  // reserved3: Flags - 6 bits reserved, 1bit ack required, 1bit res required
  _message[_packetLength++] = (lowByte(0x01));  //forcing a repsonse

  // Sequence, unimplemented
  _message[_packetLength++] = (lowByte(0x00));

  //// PROTOCOL HEADER
  // real bulbs send timestamp?  docs say "reserved"
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

  // async packets get a free write back to the original object
  Udpi.write(_message, _packetLength);

  // debugging output
  debug_print(F("Packet processed: "));
  for (int j = 0; j < _packetLength; j++) {
	if(_message[j] <= 0x0F) { debug_print(F("0"));}  // pad with zeros for proper alignment
	debug_print(_message[j], HEX);
	debug_print(SPACE);
  }
  //printLifxPacket(pkt, _packetLength);
  debug_println();
  return LifxPacketSize + pkt.data_size;
}

// this function sets the lights based on values in the globals
// TODO: refactor to take parameters instead of globals 
void setLight() {
  if( UPDATING == 1) { return; }  // cheesey update blocker
  else { UPDATING = 1; }
  int maxColor = 255;
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
  //auto call = lifx->turn_on();

  if (power_status && bri) {
	float bright = (float)bri/65535;
	// if we are setting a "white" colour (kelvin temp)
	if (sat < 1) { //removed condition: kel > 0, app seems to always send kelvin but sets saturation to 0
	  auto callW = white_led->turn_on();
	  auto callC = color_led->turn_off();
      uint16_t mireds = 1000000 / kel;  

      // convert kelvin to RGB
	  //rgbb kelvin_rgb;
	  //kelvin_rgb = kelvinToRGB(kel);

	  // convert the RGB into HSV
	  //hsv kelvin_hsv;
	  //kelvin_hsv = rgb2hsv(kelvin_rgb);

	  // set the new values ready to go to the bulb (brightness does not change, just hue and saturation)
	  //this_hue = map(kelvin_hsv.h, 0, 359, 0, 767);
	  //this_sat = map(kelvin_hsv.s * 1000, 0, 1000, 0, 255); //multiply the sat by 1000 so we can map the percentage value returned by rgb2hsv
	  callW.set_color_temperature(mireds);
	  callW.set_brightness(bright);
	  callW.set_transition_length(dur);
	  callC.perform();
	  callW.perform();
	} else {
	int this_hue = map(hue, 0, 65535, 0, 767);
	int this_sat = map(sat, 0, 65535, 0, 255);
	int this_bri = map(bri, 0, 65535, 0, 255);
	auto callW = white_led->turn_off();
	auto callC = color_led->turn_on();
		uint8_t rgbColor[3];
		hsb2rgb(this_hue, this_sat, this_bri, rgbColor);
		debug_println(F("Colors:"));
		debug_print(F(" Red:"));
		debug_print(rgbColor[0]);
		debug_print(F(" Green:"));
		debug_print(rgbColor[1]);
		debug_print(F(" Blue:"));
		debug_print(rgbColor[2]);
		debug_println();
		float r = (float)rgbColor[0] / maxColor;
		float g = (float)rgbColor[1] / maxColor;
		float b = (float)rgbColor[2] / maxColor;
		// LIFXBulb.fadeHSB(this_hue, this_sat, this_bri);
		callC.set_rgb(r,g,b);
		callC.set_brightness(bright);//(bri/65535);
		callC.set_transition_length(0);
		callW.perform();
		callC.perform();
	}
  }  else { // shit be off, yo
	auto call = color_led->turn_off();
	call.set_rgb(0,0,0);
	call.set_brightness(0);
	call.set_transition_length(dur);
 	call.perform();
	auto call2 = white_led->turn_off();
	call2.perform();
	// LIFXBulb.fadeHSB(0, 0, 0);
  }
  UPDATING = 0;
}

//#define DEG_TO_RAD(X) (M_PI*(X)/180)

void hsi2rgbw(float H, float S, float I, int* rgbw) {
  int r, g, b, w;
  float cos_h, cos_1047_h;
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
  
  if(H < 2.09439) {
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    r = S*255*I/3*(1+cos_h/cos_1047_h);
    g = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    b = 0;
    w = 255*(1-S)*I;
  } else if(H < 4.188787) {
    H = H - 2.09439;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    g = S*255*I/3*(1+cos_h/cos_1047_h);
    b = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    r = 0;
    w = 255*(1-S)*I;
  } else {
    H = H - 4.188787;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    b = S*255*I/3*(1+cos_h/cos_1047_h);
    r = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    g = 0;
    w = 255*(1-S)*I;
  }
  
  rgbw[0]=r;
  rgbw[1]=g;
  rgbw[2]=b;
  rgbw[3]=w;
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
