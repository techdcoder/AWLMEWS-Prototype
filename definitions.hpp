#ifndef DEFINITIONS_HEADER
#define DEFINITIONS_HEADER

#define BAUDRATE 9600
#define DEBUG

bool autoConnectWifi = true;
bool autoConnectServer = false;

#define WIFI_NAME "HUAWEI-8E14"
#define WIFI_PASSWORD "58617651"

//#define WIFI_NAME "FTTH0-4A3CD0"
//#define WIFI_PASSWORD "chochiebaby27"

#define SERVER_PORT 3600
#define SERVER_IP  "192.168.1.39"

//#define SERVER_PORT 3600
//#define SERVER_IP 192.168.8.170

#define IOT_CHANNEL 2291417
#define IOT_WRITE_KEY "ZGTZIEYBOGM3ZGWF"

enum UNIT {MEASUREMENT_MM,MEASUREMENT_CM};

void haltProgram(){
  while(true) {}
}

#endif
