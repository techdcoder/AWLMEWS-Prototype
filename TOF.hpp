#ifndef TOF_HEADER
#define TOF_HEADER

#include "definitions.hpp"

#include <VL53L0X.h>

class TOF{
private: 
  VL53L0X tof;

  int timingBudget = 20000;
  float signalRateLimit = 0.25;
  int sampleSize = 1; 

  bool virtualMode = false; 
  float baseline, upOffset, downOffset;

  void applySettings(){
    tof.setMeasurementTimingBudget(timingBudget);
    tof.setSignalRateLimit(signalRateLimit);
  }

  bool filter(UNIT unit, float value){
    if(unit == MEASUREMENT_MM){
      return value > 80000.0;
    }else{
      return value > 800.0;
    }
  }

public: 
  TOF() {}

  void init(){
    tof.setTimeout(50000);
    tof.setAddress(0x25);

    if(!tof.init()){
      Serial.println("Failed to initialize TOF Sensor");
      haltProgram();
    }
  }

  void initVirtual(float baseline_, float upOffset_, float downOffset_){
    virtualMode = true;

    baseline = baseline_;
    upOffset = upOffset_;
    downOffset = downOffset_;
  }

  void changeSettings(int sampleSize_, int timingBudget_, float signalRateLimit_){
    if(virtualMode) return;
    sampleSize = sampleSize_;
    timingBudget = timingBudget_;
    signalRateLimit = signalRateLimit_;

    applySettings();
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

    float tofReading = tof.readRangeSingleMillimeters();
    if(unit == MEASUREMENT_MM){
      return tofReading; 
    }
    return tofReading / 10.0;
  }

  float getReading(UNIT unit){
    float sum = 0.0;
    for(int i = 0; i < sampleSize; ++i){
        float reading = getIndividualReading(unit);
        if(filter(unit,reading)){
          i--;
          continue;
        }
        sum += reading;
    }
    return sum / sampleSize;
  }

};

#endif 