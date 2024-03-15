#include <VL53L0X.h>
#include "definitions.hpp"
#include <DS3231.h>
#include <Wire.h>
#include "ESP8266WiFi.h"
#include <ThingSpeak.h>
#include "Ultrasonic.h"
#include "secrets.hpp"

VL53L0X tofSensor;
DS3231 rtc;
WiFiClient client;
Ultrasonic ultrasonicSensor(D6,D5);

void haltProgram(){
  while(true) {}
}

namespace SETTINGS{
  int ultrasonicSamples = 1;
  int tofSamples = 1;
  int tofSignalRangeLimit = 0.25;
  int tofTimingBudget = 20000;
};

void applySettings(){
  tofSensor.setSignalRateLimit(SETTINGS::tofSignalRangeLimit);
  tofSensor.setMeasurementTimingBudget(SETTINGS::tofTimingBudget);
}


WiFiClient server;

const int serverPort = 3605;
const char* serverIP = "192.168.1.39";

void connectToServer(){
  while(true){
    if(!server.connect(serverIP,serverPort)){
      Serial.print("Cannot connect to server on ");
      Serial.print(serverIP);
      Serial.print(":");
      Serial.println(serverPort);
      delay(200);
    }else{
      Serial.print("Connected to Server");
      return;
    }
  }
} 

void setupWifi(){
  WiFi.begin(WIFI_NAME,WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Connecting to Wifi");
    delay(100);
  }
  Serial.println("Connected to Wifi");
  Serial.println("Connecting to Server");
  connectToServer();
}

void setupTof(){

  tofSensor.setTimeout(5000);
  tofSensor.setAddress(0x25);

  applySettings();
}


void setup() {
  Serial.begin(BAUDRATE);
  
  setupWifi();

  //ThingSpeak.begin(client);
  Wire.begin();

  setupTof();
}

void testTof(){

  float tofReading = tofSensor.readRangeSingleMillimeters() / 10.0;

  Serial.print("TOF: ");
  Serial.println(tofReading);

  delay(50);
}

void testUltrasonic(){
  float ultrasonicReading = ultrasonicSensor.read();

  Serial.print("Ultrasonic:  ");
  Serial.println(ultrasonicReading);
}


void checkConnection(){
  if(!server.connected()){
    Serial.println("Connection Lost! Reconnecting");
    connectToServer();
  }
}

void updateSettings(char *cmd){
  Serial.println("Update Settings");
  cmd = strtok(cmd," ");
  cmd = strtok(NULL," ");
  SETTINGS::ultrasonicSamples = atoi(cmd);
  
  cmd = strtok(NULL," ");  
  SETTINGS::tofSamples = atoi(cmd);

  cmd = strtok(NULL," ");  
  SETTINGS::tofSignalRangeLimit = atoi(cmd);

  cmd = strtok(NULL," ");  
  SETTINGS::tofTimingBudget = atoi(cmd);
  
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
      float readingsAvg = 0.0;
      for(int j = 0; j < SETTINGS::ultrasonicSamples; ++j){
        float ultrasonicReading = ultrasonicSensor.read();
        readingsAvg += ultrasonicReading;
      }
      readingsAvg /= SETTINGS::ultrasonicSamples;
      Serial.println(readingsAvg);
      server.println(readingsAvg);
      server.flush();
    }
  }else{
    Serial.println("TOF Sensor");
    Serial.print("TOF Samples: ");
    Serial.println(SETTINGS::tofSamples);
    for(int i = 0; i < sampleSize; ++i){  
      float readingsAvg = 0.0;
      for(int j = 0; j < SETTINGS::tofSamples; ++j){
        float tofReading = tofSensor.readRangeSingleMillimeters() / 10.0;
        if(tofReading > 800){
          j--;
          continue;
        }
        readingsAvg += tofReading;
      }
      readingsAvg/=SETTINGS::tofSamples;
      server.println(readingsAvg);
      server.flush();
    }
  }
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
    Serial.println(buf);
  }

  if(buf[0] == 's'){
    updateSettings(buf);
  }else if(buf[0] == 't'){
    testSensors(buf);
  }
}

void loop() {
  checkConnection();
  processCommand();
}
