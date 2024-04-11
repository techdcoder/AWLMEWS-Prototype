  #include "definitions.hpp"
#include <DS3231.h>
#include <Wire.h>
#include "ESP8266WiFi.h"
#include <ThingSpeak.h>
#include "TOF.hpp"
#include "SerialReader.hpp"
#include "UltrasonicSensor.hpp"

using namespace std;

TOF tof;
DS3231 rtc;
WiFiClient client;
UltrasonicSensor ultrasonic;
WiFiClient server;
bool demoMode = false;
const int siren = D4;

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

struct Time{
  int hour, minute, second, millisecond;
};

int offsetMillis = 0;
int getMillis(){
  return (millis() - offsetMillis) % 1000;
}

void setTime(Time t){
  rtc.setHour(t.hour);
  rtc.setMinute(t.minute);
  rtc.setSecond(t.second);
  offsetMillis = t.millisecond - getMillis();
}

void updateOffset(){
  int current = millis();
  int prevSec = rtc.getSecond();
  while(millis() - current < 2000){
    int s = rtc.getSecond();
    if(s != prevSec){
      offsetMillis = millis() % 1000;
      prevSec = s;
    }
  }
}

Time addTime(Time t, int m){
  t.millisecond += m;
  if(t.millisecond >= 1000){
    t.millisecond %= 1000;
  }

  if(t.second >= 60){
    t.minute++;
    t.second %= 60;
  }

  if(t.minute >= 60){
    t.hour++;
    t.minute %= 60;
  }
  return t;
}

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
      yield();
      delay(200);
    }else{
      Serial.print("Connected to Server");
      return;
    }
  }
} 


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
        yield();
      }
  }
  Serial.println(buffer);

}

void clearSerial(){
  while(Serial.available() > 0){
    Serial.read();
    yield();
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
    yield();
  }
  Serial.println("Connected to Wifi");
  Serial.println("Connecting to Server");
  connectToServer();
}


void setup() {
  delay(100);
  Serial.begin(BAUDRATE);
  
  setupWifi();

  ThingSpeak.begin(client);
  Wire.begin();

  tof.init();
  ultrasonic.init(D6,D5);

  updateOffset();
 
 

  pinMode(siren,OUTPUT);
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



void updateTime(char* str){
  unsigned long long beginTime = millis();
  server.println("f");
  while(server.available() <= 0) {
    yield();
   }
  unsigned long long endTime = millis();
  readFromServer(str,256);
  Serial.println(str);

  Time t;

  t.hour = atoi(strtok(str, " "));
  t.minute = atoi(strtok(NULL, " "));
  t.second = atoi(strtok(NULL, " "));
  t.millisecond = atoi(strtok(NULL, " "));
  t = addTime(t,(endTime-beginTime)/2);

  setTime(t);
}

Time getTime(){
    Time t;
    t.second = rtc.getSecond();
    t.minute = rtc.getMinute();

    bool h, hpm;
    t.hour = rtc.getHour(h,hpm);
    if(h){
      t.hour += hpm * 12;
    }
    t.millisecond = getMillis();
    return t;
}

int readFromServer(char* buf, int maxSize){
  memset(buf, 0, maxSize);
  int size = 0; 
  while(server.available() > 0){
    char ch = server.read();
    if(ch == '\n') break;
    buf[size++] = ch;
    
    yield();
    delay(10);
  }
  buf[size] = '\0';
  return size;
}



void testLatency(char *cmd){
  Serial.println("Latency Test");
  
  cmd = strtok(cmd, " ");
  cmd = strtok(NULL, " ");
  int samples = atoi(cmd);
  cmd = strtok(NULL," ");
  int sensorType = atoi(cmd);

  if(sensorType == 0){
    ultrasonic.getReading(MEASUREMENT_CM);
  }else{
    tof.getReading(MEASUREMENT_CM);
  }

  server.println("f");
  
  Serial.print("SAMPLES: ");
  Serial.println(samples);

  char buf[256];
  for(int i = 0; i < samples; ++i){
      if(server.available() > 0){
        delay(50);
      }
      int len = readFromServer(buf,256);
      if(len < 2){
        i--;
        continue;
      }
    
    if(sensorType == 0){
      ultrasonic.getReading(MEASUREMENT_CM);
    }else{
      tof.getReading(MEASUREMENT_CM);
    }
    server.println("f");
    Serial.println("Feedback");
  }
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

      Serial.println(reading);
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


      Serial.println(reading);
      server.println(reading);

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

  float maximumReading = max(ultrasonicReading,tofReading);
  if(SETTINGS::sirenEnabled && maximumReading >= SETTINGS::threshold){
    digitalWrite(siren,HIGH);
    Serial.println("SIREN ACTIVATED");
  }else{
    digitalWrite(siren,LOW);
    Serial.println("SIREN OFF");
  }
  printSettings();
  
}

void processCommand(){
  char buf[256];  
  int size = readFromServer(buf,256);
  if(size){
    Serial.println(" ");
    Serial.print("COMMAND: ");
    Serial.println(buf);
  }else{
    return;
  }

  if(buf[0] == 's'){
    updateSettings(buf);
  }else if(buf[0] == 't'){
    testSensors(buf);
  }else if(buf[0] == 'd'){
    bool st = buf[2]-'0';
    demoMode = st;
    Serial.println(buf);
  }else if(buf[0] == 'm'){
     updateTime(buf);
  }else if(buf[0] == 'l'){
    testLatency(buf);
  }else{  
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
