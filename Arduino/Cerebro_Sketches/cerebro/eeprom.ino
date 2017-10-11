
void updateWaveform(uint16_t (&marks)[NUMPULSES]){
  //Record the current waveform parameters to the eeprom log. This way we can recall the previous parameters when looking back on a log that contains mid-session parameter changes
  eepromWriteByte(address,'P');
  address++;
  for (byte m = 0; m<NUMPARAM; m++){
    eepromWriteByte(address,waveform[m]>>8);
    eepromWriteByte(address+1,waveform[m] & 255);
    address+=2;
  }
  //Convert the received marks into integers and set them as the waveform parameters
  for (int k = 0; k<NUMPARAM; k++){
    waveform[k] = convertBIN(marks,16,7+16*k);
  }
  //Save the freshly updated waveform parameters to the designated parameter block (addresses 2-9) so they can be recalled when cerebro is turned off between sessions
  for (byte m = 0; m<NUMPARAM; m++){
    eepromWriteByte(2*m+2,waveform[m]>>8);
    eepromWriteByte(2*m+3,waveform[m] & 255);
  }
  printParameters();
}

void updateFadeVector(uint16_t (&marks)[NUMPULSES],byte offset){
  unsigned int fadeValue  = 0;
  for (byte m = 0; m<NUMPARAM; m++){    //write values to eeprom
    fadeValue = convertBIN(marks,16,7+16*m);
    eepromWriteByte(FADE_START+offset+2*m,fadeValue>>8);
    eepromWriteByte(FADE_START+offset+2*m+1,fadeValue & 255);
  }
  powerLevel = word(eepromReadByte(FADE_START)<<8|eepromReadByte(FADE_START+1)); //read powerLevel from eeprom
}

void updateHardware(uint16_t (&marks)[NUMPULSES]){
  unsigned int hardwareValue  = 0;
  for (byte m = 0; m<2; m++){   //only care about 2 values, Cerebro # and Laser Diode #
    hardwareValue = convertBIN(marks,16,7+16*m);
    eepromWriteByte(2*m+12,hardwareValue>>8);
    eepromWriteByte(2*m+13,hardwareValue & 255);
  }
}

unsigned int convertBIN(uint16_t (&marks)[NUMPULSES],byte numMarks=4,byte start=0){
  byte trueThresh = 225;
  unsigned int key = 0;
  for (int i = 0; i <numMarks; i++){
    key = key<<1 | marks[start + i]>trueThresh;
  }
  return key;
}

void recordEvent(byte letter){
  unsigned long temp = millis();
  eepromWriteByte(address , letter);
  eepromWriteByte(address + 1, temp >>16 & 255);
  eepromWriteByte(address + 2, temp >> 8 & 255 );
  eepromWriteByte(address + 3, temp & 255);
  address+=4;
}

void recordLightLevel(int lightLevel){
  unsigned long temp = millis();
  eepromWriteByte(address , 'L');
  eepromWriteByte(address + 1, lightLevel >> 8 & 255 );
  eepromWriteByte(address + 2, lightLevel & 255);
  address+=3;
}

void eepromWriteByte( unsigned int writeAddress, byte data ) {
  TinyWireM.beginTransmission(0x50);
  TinyWireM.write(writeAddress >> 8);  // MSB
  delay(1);
  TinyWireM.write(writeAddress & 255); // LSB
  delay(1);
  TinyWireM.write(data);
  delay(1);
  TinyWireM.endTransmission();
  delay(1);
}

byte eepromReadByte( unsigned int readAddress ) {  //"Random Read" read operation from FT24C64A Two-Wire Serial EEPROM datasheet
  //dummy write instruction
  TinyWireM.beginTransmission(0x50);
  TinyWireM.write(readAddress >> 8);  // MSB
  TinyWireM.write(readAddress & 255); // LSB
  TinyWireM.endTransmission();
  //"Current Address Read" read operation from datasheet
  TinyWireM.requestFrom(0x50,1);
  while(!TinyWireM.available()){
    ;//wait
  }
  return TinyWireM.receive();
}

void save2EEPROM(){
  //in the first 2 bytes of memory store the last address used
  eepromWriteByte(0,address>>8);
  eepromWriteByte(1,address & 255);
}

void readAddresses(int start, int finish){
  long stamp;
  long first;
  unsigned int second;
  byte third;
  for (int k = start; k < finish ; k++) {  //print the list of events.
    first = eepromReadByte(k+1);
    second = eepromReadByte(k+2);
    third = eepromReadByte(k+3);
    if(char(eepromReadByte(k))=='T'){
      stamp = first<<16|second<<8|third;                    //combine bytes to get timestamp
      mySerial.print('\r');
      mySerial.print(stamp);
      mySerial.print(',');
      mySerial.print(F("trigger"));
      k+=3;
    }
    else if (char(eepromReadByte(k))=='A'){
      stamp = first<<16|second<<8|third;
      mySerial.print('\r');
      mySerial.print(stamp);
      mySerial.print(',');
      mySerial.print(F("abort"));
      k+=3;
    }
    else if (char(eepromReadByte(k))=='C'){
      stamp = first<<16|second<<8|third;
      mySerial.print('\r');
      mySerial.print(stamp);
      mySerial.print(',');
      mySerial.print(F("continue"));
      k+=3;
    }
    else if (char(eepromReadByte(k))=='L'){
      int lightLevel = first<<8|second;
      mySerial.print(',');
      mySerial.print(lightLevel);
      k+=2;
    }
    else if (char(eepromReadByte(k))=='P'){
      unsigned int param1;
      for (int i = 0; i<NUMPARAM; i++){
        mySerial.print('\r');
        param1 = eepromReadByte(k+1+2*i)<<8;
        strcpy_P(label, (char*)pgm_read_word(&(parameterLabels[i])));    //Necessary casts and dereferencing
        mySerial.print(label);
        mySerial.print(word(param1|eepromReadByte(k+2*(i+1))));
      }
      k+=NUMPARAM*2;
    }
    else{
      mySerial.println(F("error"));
    }
  }
}

void printEEPROM(){
  //print firmware version
  mySerial.print(F("Ver,"));
  mySerial.print(version);

  //print hardware parameters
  mySerial.print('\r');
  mySerial.print(F("Cerebro,"));
  mySerial.print(cerebroNum);
  mySerial.print('\r');
  mySerial.print(F("LD,"));
  mySerial.print(LD);

  //print the list of events
  unsigned int endingAddress = (eepromReadByte(0)<<8|eepromReadByte(1));
  readAddresses(LOG_START,endingAddress);

  //print pulse parameters
  for (int i = 0; i<NUMPARAM; i++){
    mySerial.print('\r');
    strcpy_P(label, (char*)pgm_read_word(&(parameterLabels[i])));
    mySerial.print(label);
    mySerial.print(waveform[i]);
  }
  mySerial.print('\r');
}

void myShift(int val){                  //shifts out data MSB first
  for (int i = 7; i > -1; i--){
    if(val & (1<<i)){                   //shift high bit
      DATA_outputReg |= (1<<DATA_pin);
    }
    else{                               // shift low bit
      DATA_outputReg &= ~(1<<DATA_pin);
    }
    CLK_outputReg |= (1<<CLK_pin);      //clock high
    CLK_outputReg &= ~(1<<CLK_pin);     //clock low
  }
}

void printParameters(){
    char delimeter = '~';
    mySerial.print('&');
    mySerial.print(version);
    mySerial.print(delimeter);
    mySerial.print(cerebroNum);
    mySerial.print(delimeter);
    mySerial.print(LD);
    mySerial.print(delimeter);
    mySerial.print(powerLevel);
    mySerial.print(delimeter);
    for (int i  = 0 ; i<NUMPARAM; i++){
      mySerial.print(waveform[i]);
      mySerial.print(delimeter) ;
    }
    mySerial.println('*');
}

void printFadeVector(){
  for (int k = FADE_START; k < FADE_START+200 ; k+=NUMPARAM*2) {
    unsigned int param1;
    for (int i = 0; i<NUMPARAM; i++){
      param1 = eepromReadByte(k+2*i)<<8;
      mySerial.print(word(param1|eepromReadByte(k+1+2*i)));
      if(i!=NUMPARAM-1){
        mySerial.print(',');
      }
    }
    mySerial.print('\r');
  }
}
