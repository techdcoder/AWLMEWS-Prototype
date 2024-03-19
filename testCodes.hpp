
void testTof(){

  float tofReading = tof.getIndividualReading(MEASUREMENT_CM);

  Serial.print("TOF: ");
  Serial.println(tofReading);

  delay(50);
}

void testUltrasonic(){
  float ultrasonicReading = ultrasonic.getIndividualReading(MEASUREMENT_CM);

  Serial.print("Ultrasonic:  ");
  Serial.println(ultrasonicReading);
}
