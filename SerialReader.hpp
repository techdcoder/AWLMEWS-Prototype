#ifndef SERIAL_READER
#define SERIAL_READER


void flushInput(){
  while(Serial.available() > 0){
    Serial.read(); 
  }
}

bool pass(char c){
  return isAlphaNumeric(c) || c == '.' || c == '-';
}

int getText(char* out, int maxSize) {
  while(Serial.available() <= 0) {}
  int index = 0;
  while(Serial.available() > 0){
    char c = Serial.read();
    if(pass(c)){
      out[index++] = c;
      if(index == maxSize) break;
    }
    delay(10);
  }
  return index;
}

void getInputPrint(char *out, int maxSize, char* text){
  Serial.println(text);
  getText(out,maxSize);
}

#endif