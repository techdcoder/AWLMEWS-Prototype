#include "definitions.hpp"
#include <DS3231.h>
#include <Wire.h>
#include "ESP8266WiFi.h"
#include <ThingSpeak.h>
#include "secrets.hpp"
#include "TOF.hpp"
#include "SerialReader.hpp"
#include "UltrasonicSensor.hpp"

TOF tof;
DS3231 rtc;
WiFiClient client;
UltrasonicSensor ultrasonic;
WiFiClient server;
bool demoMode = false;

namespace SETTINGS{
  int ultrasonicSamples = 1;
  int tofSamples = 1;
  float tofSignalRangeLimit = 0.25;
  int tofTimingBudget = 20000;
  float prototypeHeight = 30.0;
  bool iotEnabled = true;
  bool sirenEnabled = true;
  float threshold = 10.0;
};

void printSettings(){
  char output[1024];
  sprintf(output,"Ultrasonic Samples: %d\n TOF Samples: %d\n TOF Signal Range Limit: %f\n TOF Timing Budget: %d\n Prototype Height: %f\n IOT Enabled: %d\n Siren Enabled: %d\n Threshold: %f",SETTINGS::ultrasonicSamples,SETTINGS::tofSamples,SETTINGS::tofSignalRangeLimit,SETTINGS::tofTimingBudget,SETTINGS::prototypeHeight,SETTINGS::iotEnabled,SETTINGS::sirenEnabled,SETTINGS::threshold);
  Serial.println(output);
}

void applySettings(){
  tof.changeSettings(SETTINGS::tofSamples, SETTINGS::tofTimingBudget, SETTINGS::tofSignalRangeLimit);
  ultrasonic.changeSettings(SETTINGS::ultrasonicSamples);
}



char wifiName[256], wifiPassword[256];
char serverIp[256];
int serverPort;
  

void connectToServer(){
  while(true){  
    if(!server.connect(serverIp,serverPort)){
      Serial.print("Cannot connect to server on ");
      Serial.print(serverIp);
      Serial.print(":");
      Serial.println(serverPort);
      delay(200);
    }else{
      Serial.print("Connected to Server");
      return;
    }
  }
} 
bool autoConnectWifi = false;
bool autoConnectServer = false;

void readSerial(char *buffer){
  delay(500);
  int counter = 0;
  while(Serial.available() <= 0) {}
  if(Serial.available() > 0){
      while(true){
        char c = Serial.read();
        if(c == '\n') break;
        if((c >= 'a' && c <= 'z' )  || (c >= 'A' && c <= 'Z')){
          buffer[counter++] = c;
        }
      }
  }
  Serial.println(buffer);

}

void clearSerial(){
  while(Serial.available() > 0){
    Serial.read();
  }
}

void setupWifi(){
  char serverPortStr[256];

  memset(wifiName,0,sizeof(wifiName));
  memset(wifiPassword,0,sizeof(wifiPassword));
  memset(serverIp,0,sizeof(serverIp));
  memset(serverPortStr,0,sizeof(serverPortStr));

  if(autoConnectWifi){
    strcpy(wifiName, WIFI_NAME);
    strcpy(wifiPassword, WIFI_PASSWORD);
  }else{
    getInputPrint(wifiName,256,"Please enter Wifi Name");
    getInputPrint(wifiPassword,256,"Please enter Wifi Password"); 
   }

  if(autoConnectServer){
    serverPort = SERVER_PORT;
    strcpy(serverIp, SERVER_IP);
  }else{
    getInputPrint(serverIp,256,"Please enter Server IP");
    getInputPrint(serverPortStr,256, "Please enter Server Port ");
    serverPort = atoi(serverPortStr);
  }

  WiFi.begin(wifiName,wifiPassword);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Connecting to Wifi");
    Serial.println(wifiName);
    Serial.println(wifiPassword);
    delay(100);
  }
  Serial.println("Connected to Wifi");
  Serial.println("Connecting to Server");
  connectToServer();
}


void setup() {
  Serial.begin(BAUDRATE);
  
  setupWifi();

  ThingSpeak.begin(client);
  Wire.begin();

  tof.init();
  ultrasonic.init(D6,D5);
}


void checkConnection(){
  if(!server.connected()){
    Serial.println("Connection Lost! Reconnecting");
    connectToServer();
  }
  server.flush();
}

void updateSettings(char *cmd){
  Serial.println("Update Settings");
  cmd = strtok(cmd," ");
  cmd = strtok(NULL," ");
  SETTINGS::ultrasonicSamples = atoi(cmd);
  
  cmd = strtok(NULL," ");  
  SETTINGS::tofSamples = atoi(cmd);

  cmd = strtok(NULL," ");  
  SETTINGS::tofSignalRangeLimit = atof(cmd);

  cmd = strtok(NULL," ");  
  SETTINGS::tofTimingBudget = atoi(cmd);
  
  cmd = strtok(NULL," ");
  SETTINGS::prototypeHeight = atof(cmd);

  cmd = strtok(NULL," ");
  SETTINGS::threshold = atof(cmd);


  cmd = strtok(NULL," ");
  SETTINGS::iotEnabled = atoi(cmd);

  cmd = strtok(NULL," ");
  SETTINGS::sirenEnabled = atoi(cmd);

  applySettings();
}

void testSensors(char *cmd){
  Serial.println("Test Sensors");
  cmd = strtok(cmd," ");
  cmd = strtok(NULL," ");

  int sampleSize = atoi(cmd);
  Serial.print("Sample Size: ");
  Serial.println(sampleSize);

  cmd = strtok(NULL," ");
  int sensorType = atoi(cmd);

  if(sensorType == 0){
    Serial.println("Ultrasonic Sensor");
    Serial.print("Ultrasnonic Samples: ");
    Serial.println(SETTINGS::ultrasonicSamples);

    for(int i = 0; i < sampleSize; ++i){
      float reading = ultrasonic.getReading(MEASUREMENT_CM);
      reading = SETTINGS::prototypeHeight - reading;

      server.println(reading);
      server.flush();
    }
  }else{
    Serial.println("TOF Sensor");
    Serial.print("TOF Samples: ");
    Serial.println(SETTINGS::tofSamples);
      
    for(int i = 0; i < sampleSize; ++i){
      float reading = tof.getReading(MEASUREMENT_CM);
      reading = SETTINGS::prototypeHeight - reading;
      server.println(reading);
      server.flush();
    }
  }
}

void demoModeRun(){
  float ultrasonicReading = ultrasonic.getReading(MEASUREMENT_CM);
  float tofReading = tof.getReading(MEASUREMENT_CM);

  ultrasonicReading = SETTINGS::prototypeHeight - ultrasonicReading;
  tofReading = SETTINGS::prototypeHeight - tofReading;

  char output[256];
  memset(output,0,sizeof(output));
  sprintf(output,"%.3f %.3f", ultrasonicReading, tofReading);

  server.println(output);
  server.flush();

  Serial.println(output);

  if(SETTINGS::iotEnabled){
    ThingSpeak.writeField(IOT_CHANNEL,1,ultrasonicReading,IOT_WRITE_KEY);
    ThingSpeak.writeField(IOT_CHANNEL,2,tofReading,IOT_WRITE_KEY);
  }
  printSettings();
}

void processCommand(){
  char buf[256];  
  memset(buf, 0, sizeof(buf));
  int size = 0;
  while(server.available()){
    char ch = server.read();
    buf[size++] = ch;
  }

  buf[size] = '\0';
  if(size){
    Serial.println(" ");
    Serial.print("COMMAND: ");
    Serial.println(buf);
  }

  if(buf[0] == 's'){
    updateSettings(buf);
  }else if(buf[0] == 't'){
    testSensors(buf);
  }else if(buf[0] == 'd'){
    bool status = buf[2]-'0';
    Serial.println(status);

    demoMode = status;
  }
}

void run(){
  if(demoMode){
    demoModeRun();
  }
}

void loop() {
  checkConnection();
  processCommand();
  run();
}
