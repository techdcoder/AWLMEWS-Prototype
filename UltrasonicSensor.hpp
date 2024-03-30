#ifndef ULTRASONIC_HEADER
#define ULTRASONIC_HEADER

#include <Ultrasonic.h>
// TODO Make sensor class, to make it easier for future maintenance. Derived class will be TOF and Ultrasonic
//    Sensor
//      |
// Ultrasonic, TOF

class UltrasonicSensor{
private:
  Ultrasonic ultrasonic = Ultrasonic(0,0);
  int sampleSize = 1;

  bool virtualMode = false; 
  float baseline, upOffset, downOffset;


public:
  UltrasonicSensor() {}

  void init(int echo, int trig){
    ultrasonic = Ultrasonic(echo,trig);
  }

  float getIndividualReading(UNIT unit){
    if(virtualMode){
      float randomValue = random(0,4096) / 4096.0 * 2.0 - 1.0;
      if(randomValue > 0){
        randomValue *= upOffset;
      }else{
        randomValue *= downOffset; 
      }
      return baseline + randomValue;
    }
    
    float ultrasonicReading = ultrasonic.read();
    if(unit == MEASUREMENT_MM){
      ultrasonicReading *= 10;
    }
    return ultrasonicReading;
  }

  void initVirtual(float baseline_, float upOffset_, float downOffset_){
    virtualMode = true;

    baseline = baseline_;
    upOffset = upOffset_;
    downOffset = downOffset_;
  }

  float getReading(UNIT unit){
    float sum = 0.0;
    for(int i = 0; i < sampleSize; ++i){
        sum += getIndividualReading(unit);
        delay(100);
    }
    return sum / sampleSize;
  }

  void changeSettings(int sampleSize_){
    sampleSize = sampleSize_;
  }

};

#endif 
