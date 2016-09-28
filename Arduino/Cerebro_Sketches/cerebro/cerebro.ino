/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MIT License

Copyright (c) 2015-2016 Andy S. Lustig

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
byte version = 38;
byte cerebroNum = 26;
byte LD = 19;
const int levels[100] PROGMEM = {//LD19 4 mW
906 , 906 , 905 , 904 , 903 , 902 , 901 , 901 , 900 , 899 , 898 , 897 , 897 , 896 , 895 ,
894 , 893 , 893 , 892 , 891 , 890 , 889 , 888 , 887 , 886 , 884 , 883 , 882 , 881 , 880 ,
878 , 877 , 876 , 874 , 873 , 871 , 870 , 869 , 868 , 866 , 865 , 864 , 863 , 861 , 860 ,
858 , 856 , 855 , 853 , 851 , 849 , 847 , 845 , 843 , 841 , 839 , 836 , 834 , 831 , 829 ,
826 , 823 , 821 , 818 , 815 , 813 , 810 , 806 , 802 , 798 , 795 , 792 , 789 , 785 , 782 ,
775 , 769 , 765 , 760 , 754 , 747 , 739 , 730 , 722 , 714 , 706 , 696 , 681 , 666 , 651 ,
633 , 615 , 596 , 574 , 552 , 504 , 499 , 499 , 498 , 498 };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// #define DEBUG       //uncomment for DEBUG MODE
// #define MCUBE
// #define STARTDELAY
// #define OLDBOARD
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*

        ______                   __
       / ____/___   _____ ___   / /_   _____ ____
      / /    / _ \ / ___// _ \ / __ \ / ___// __ \
     / /___ /  __// /   /  __// /_/ // /   / /_/ /
     \____/ \___//_/    \___//_.___//_/    \____/

When an IR emitter sends a burst of IR light flickering at 38khz, an IR sensor can detect the burst and output
a corresponding low signal (mark). When the IR sensor isn't detecting these bursts of flickering IR light,
it is outputting a high signal (space). We are trying to send a message wirelessly using bursts of flickering
IR light from an IR led on Base Station. The IR sensor on Cerebro will detect these bursts and output a sequence of corrseponding marks
and spaces. Unique messages can be sent using patterns of marks and spaces with particular durations.

Cerebro is waiting for a train of marks from the IR sensor. If it receives exactly four marks of certain lengths,
(long, short, long, short), it will provide power to the onboard laser diode. If it receives exactly 87 marks,
it will check if the first 7 marks are of particular lengths (long, long, short, short , long, short, long). If they
match, the next 80 marks will be converted to binary based on their durations (long mark=1, short mark=0). The 80 bits
are converted to 5 separate 16 bit unsigned integers (0-65535) and then assigned as new pulse parameters. If Cerebro receives exactly 26
marks, it will write the most recently used memory address to the first 2 bytes of the memory. When the log is being printed, it will
know which address to stop at by looking at the first 2 bytes of memory.

listenForIR() function is based on code from Adafruit IR sensor tutorial: https://learn.adafruit.com/ir-sensor/reading-ir-commands
Adafruit code: https://github.com/adafruit/IR-Commander/blob/master/ircommander.pde
Detailed explanation of how infrared (IR) remotes work: http://www.sbprojects.com/knowledge/ir/index.php#top
Directions for uploading can be found at http://cerebro.readthedocs.org/en/latest/#
*/
#include <SoftwareSerial.h>
#include <TinyWireM.h>        // Library for ATtiny hardware I2C communication using the USI pins https://github.com/adafruit/TinyWireM
#include <avr/pgmspace.h>
SoftwareSerial mySerial(7,5); //rx,tx pin6 pin8 on attiny84

#ifdef DEBUG
//---------------DEBUG PARAMETERS-----------------------
const int memorySize =     10000;
#else
//--------------OPERATING PARAMETERS--------------------
const int memorySize =     8169;   /*The number of bytes we will save to on the EEPROM. Each event recording is 4 bytes of data
(3 for the timestamp integer and 1 for the event description character). Parameter changes are 9 bytes, 1 for the event description character
and 8 for the former parameters.8192 bytes will provide space for 2730 events to be recorded.*/
#endif

//IR sensor input//
#define IR_dirReg         DDRA
#define IR_inputReg       PINA
#define IR_pin            1       //attiny84 pin12
//BTN  input//
#define BTN_dirReg        DDRA
#define BTN_outputReg     PORTA
#define BTN_inputReg      PINA
#define BTN_pin           0       //attiny84 pin13
//DAC pins setup//
#define DATA_dirReg       DDRB
#define DATA_outputReg    PORTB
#define DATA_pin          0       //attiny84 pin2
#define CLK_dirReg        DDRB
#define CLK_outputReg     PORTB
#define CLK_pin           1       //attiny84 pin3
#define LATCH_dirReg      DDRA
#define LATCH_outputReg   PORTA
#define LATCH_pin         3       //attiny84 pin10
// //Output to dunce board setup//
// #define DUNCE_dirReg      DDRB
// #define DUNCE_outputReg   PORTB
// #define DUNCE_pin         2        //attiny84 pin5

const unsigned int maxpulselength = 1000;//maximum pulse length (in microseconds) that we will listen for. A signal longer than this maxpulselength is not a pulse and indicates the message is over
const byte NUMPULSES = 88;               //maximum length of the message we can receive
const byte irResolution = 4;             //# of microseconds that we will delay before checking the state of the IR sensor again
#define NUMPARAM 5
const char string_0[] PROGMEM = "Power Level\t,";
const char string_1[] PROGMEM = "On Time\t,";
const char string_2[] PROGMEM = "Off Time\t,";
const char string_3[] PROGMEM = "Train Duration\t,";
const char string_4[] PROGMEM = "Ramp Down\t,";
// Then set up a table to refer to your strings.
const char* const parameterLabels[] PROGMEM = {string_0, string_1, string_2, string_3, string_4};
char buffer[20];            // make sure this is large enough for the largest string it must hold
unsigned int waveform[NUMPARAM] = {};
unsigned int onDelay = 2000;
byte marksReceived;
unsigned int address = 12;  /*start at the thirteenth byte address of eeprom. We reserve the first 2 bytes for an integer indicating
                            the final address used in the last session. The next 10 bytes are reserved for waveform parameters*/
bool trigMatch = false;
bool stopMatch = false;
int error;
const float KP = 0.2;
const byte interval = 3;
bool isSettled = false;
bool isMaxed = false;
bool calibrateMode = false;
const byte indicatorLED = 8; //attiny pin 5, Arduino language pin8

byte definitions = 0;
#ifdef MCUBE
int DAClevel = 725;
#else
int DAClevel = 0;
#endif

byte listenForIR(int timeout=0);

void setup() {
  ///////////Analog setup////////////////////////////
  ADMUX = 2;       //PA2 is pin 11 on ATtiny84. ADMUX explained in SECTION 16.13 of datasheet
  ADCSRA |= (1<<ADPS2) | (1<<ADPS1);  //set division factor of 64. ADC frequncy = 8Mhz/64=125khz (ADC needs to be in 50-200khz range)
  ADCSRA |= (1<<ADEN);                //enable the ADC
  ////////////Digital setup//////////////////////////
  IR_dirReg &= ~(1<<IR_pin);          //IR sensor pin as input
  BTN_dirReg &= ~(1<<BTN_pin);        //BTN pin as input
  BTN_outputReg |= (1<<BTN_pin);      //Enable pullup
  DATA_dirReg |= (1<<DATA_pin);       //Output
  CLK_dirReg  |= (1<<CLK_pin);        //Output
  LATCH_dirReg |=  (1<<LATCH_pin);    //Output
  LATCH_outputReg |= (1<<LATCH_pin);  //Set Chip Select HIGH (LOW selects the chip)
  // DUNCE_dirReg |= (1<<DUNCE_pin);     //Output
  // DUNCE_outputReg &= ~(1<<DUNCE_pin); //start Dunce ouput LOW
  pinMode(indicatorLED,OUTPUT); //analog output'
  digitalWrite(indicatorLED,LOW);

  mySerial.begin(115200);
  TinyWireM.begin();

  #ifdef DEBUG
  definitions+=4;
  #endif
  #ifdef MCUBE
  definitions+=1;
  #endif

  mySerial.println();
  //Get the saved parameters from EEPROM
  for (int i = 0; i<NUMPARAM; i++){
    waveform[i] = word(eepromReadByte(2*i+2)<<8|eepromReadByte(2*i+3));
  }
  //if the BTN is pressed (Low Signal) when cerebro starts up then print the log from the eeprom
  #ifdef OLDBOARD
  if((BTN_inputReg & (1<<BTN_pin))){
  #else
  if(!(BTN_inputReg & (1<<BTN_pin))){
  #endif
    printEEPROM();
  }
  //otherwise just print the waveform parameters
  else{
    char delimeter = '~';
    mySerial.print(version);
    mySerial.print(delimeter);
    mySerial.print(cerebroNum);
    mySerial.print(delimeter);
    mySerial.print(LD);
    mySerial.print(delimeter);
    mySerial.print(definitions, BIN);
    mySerial.print(delimeter);
    for (int i  = 0 ; i<NUMPARAM; i++){
      mySerial.print(waveform[i]);
      mySerial.print(delimeter) ;
    }
    mySerial.println("*");
  }
}

void loop() {
  marksReceived = listenForIR();      //wait for IR signal and return the number of marks received
  if (trigMatch) {                    //trigger light upon receiving exactly 4 marks of with durations that match a key
    trigMatch = false;
    triggerEvent(waveform[0]);
  }
  else if(calibrateMode){
    digitalWrite(indicatorLED,HIGH);
    calibrateRoutine();
    calibrateMode = false;
    digitalWrite(indicatorLED,LOW);
  }
  else if (marksReceived == 26) {         //save data to EEPROM upon receiving exactly 26 marks
    save2EEPROM();
  }
}

void calibrateRoutine(){
  delay(5000);
  for (int b = 500; b<751; b+=50){
    triggerEvent(b);
    delay(15000);
  }
  for (int b = 760; b<901; b+=10){
    triggerEvent(b);
    delay(15000);
  }
  for (int b = 905; b<1026; b+=5){
    if(!isMaxed){
      triggerEvent(b);
      delay(15000);
    }
  }
}

void triggerEvent(unsigned int desiredPower){
  isSettled = false;
  unsigned long onClock,offClock,trainClock,delayClock,alt=0;
  bool laserEnabled = true; //set flag for entering waveform loop
  bool newPulse = true;      //
  bool triggerRecorded = false;
  onClock=trainClock=delayClock=millis();              //reset clocks
  byte rcvd = 0;
  unsigned int onDur = calibrateMode ? 2000 : waveform[1];
  #ifdef STARTDELAY
  while ((millis()-delayClock)<onDelay){
    if (!(IR_inputReg & (1<<IR_pin))){
      while(! (IR_inputReg & (1<<IR_pin))){
        //wait until stop pulse is finished
      }
      rcvd = listenForIR(5000);
      if (stopMatch){
        laserEnabled = laserOFF();
        if (address < memorySize) {       //record abort event
          recordEvent('A');
        }
        stopMatch = false;
      }
      rcvd = 0;
    }
  }
  #endif
  while(laserEnabled){
    //check if another command (abort or continuation) has been sent since the trigger was activated
    if (!(IR_inputReg & (1<<IR_pin))){
      while(! (IR_inputReg & (1<<IR_pin))){
        //wait until stop pulse is finished
      }
      rcvd = listenForIR(5000);
      if (trigMatch){
        onClock = millis();
        trainClock = millis();
        if (address < memorySize) {       //record continue event
          recordEvent('C');
        }
        trigMatch = false;
      }
      else if (stopMatch){
        if (waveform[4]>0){
          fade();
        }
        laserEnabled = laserOFF();
        if (address < memorySize) {       //record abort event
          recordEvent('A');
        }
        stopMatch = false;
      }
      rcvd = 0;
    }
    //else if onClock hasn't expired, turn on/keep on the laser
    else if ((millis()-onClock)<onDur){
      #ifdef MCUBE
      sendDAC(waveform[0]);
      #else
      sendDAC(DAClevel);                //Laser on
      if(alt%interval==0){              //it takes time for the photocell to respond, so only implement feedback every fourth loop
        feedback(desiredPower);         //increase or decrease DAClevel to reach desired lightPower
      }
      alt++;
      #endif
      if (!triggerRecorded){           //event has not yet been recorded
        if (address < memorySize) {    //record trigger event
          recordEvent('T');
        }
        // DUNCE_outputReg |= (1<<DUNCE_pin); //Dunce ouput HIGH
        triggerRecorded = true;
      }
      offClock = millis();
    }
    //else if offClock hasn't expired, turn off/keep off light
    else if((millis()-offClock)<waveform[2]){
      if (newPulse){                   //if the laser is on then turn it off, otherwise do nothing (i.e. leave turned off)
        newPulse = laserOFF();         //laserOn = false
      }
    }
    //else if trainClock hasn't expired, restart the light pulse
    else if((millis()-trainClock)<waveform[3]){
      newPulse = true;
      onClock = millis();
    }
    else{
      #if  !defined(MCUBE)
      if (waveform[4]>0 && !calibrateMode){
        fade();
      }
      #endif
      if(calibrateMode){
        mySerial.print(",");
        mySerial.println(DAClevel);
      }
      laserEnabled = laserOFF();
    }
  }
}

void feedback(int setPoint){
  ADCSRA |= (1<<ADSC);                    //start analog conversion
  loop_until_bit_is_clear(ADCSRA,ADSC);   //wait until conversion is done
  int resistance = ADC;
  error = setPoint-resistance;
  if(calibrateMode){
    if (!isSettled && abs(error)<7){
      isSettled = true;
      mySerial.print(setPoint);
      mySerial.print(",");
      mySerial.print(DAClevel);
    }
  }
  DAClevel = DAClevel+int(error*KP);
  if (DAClevel>4095) {
    DAClevel = 4095;
  }
  else if (DAClevel<0){
    DAClevel = 0;
  }
}

void fade(){
  unsigned long fadeClock;
  for(int i = 0; i<100; i++){
    fadeClock = millis();
    feedback(pgm_read_word_near(levels + i));
    sendDAC(DAClevel);
    while((millis()-fadeClock)<(waveform[4]/100)){
      //wait
    }
  }
}

byte listenForIR(int timeout=0) {
  /*This function builds an array of high pulse and low pulse lengths
  When it receives a pulse, it starts counting in 2 microsecond (the resolution) increments
  then stores that sum in an array. We can later estimate how long a IR pulse was by mulitplying this sum by the resolution.
  If at any point the pulse length is too long, we know the there are no more pulses coming for that particular message,
  so we should exit */
  byte pulsePairIndex = 0;
  byte trueThresh = 225;
  byte falseThresh = 120;
  uint16_t marks[NUMPULSES];    //array that stores the marks lengths
  while (1) {
    uint16_t spaceLength = 0;
    uint16_t markLength = 0;
    while (IR_inputReg & (1 << IR_pin)) {    //while the pin is high, count how long it is high by adding to spaceLength
      spaceLength++;
      delayMicroseconds(irResolution);
      if (pulsePairIndex==4){
        if (marks[0]>trueThresh && marks[1]<falseThresh && marks[2]>trueThresh && marks[3]<falseThresh){     //We received a trigger message (High,Low,High,Low)
          trigMatch = true;
          return pulsePairIndex;
        }
        else if(marks[0]<falseThresh && marks[1]>trueThresh && marks[2]>trueThresh && marks[3]<falseThresh){ //we received stop message (Low,High,High,Low)
          stopMatch = true;
          return pulsePairIndex;
        }
      }
      if((spaceLength >= (maxpulselength + timeout) && (pulsePairIndex != -1*timeout)) || pulsePairIndex == NUMPULSES) { //if the space is too long, the message is over. Either we received a message with new paramters or we received a message that we don't understand
        delayMicroseconds(1);//Don't know why this is needed, but it is....
        if (pulsePairIndex==87 && marks[0]>trueThresh && marks[1]>trueThresh && marks[2]<falseThresh  && marks[3]<falseThresh  && marks[4]>trueThresh && marks[5]<falseThresh && marks[6]>trueThresh ){ //We received new Parameters!
          //Record the current pulse parameters to the eeprom log. This way we can recall the previous parameters when looking back on a log that contains mid-session parameter changes
          eepromWriteByte(address,'P');
          address++;
          for (byte m = 0; m<NUMPARAM; m++){
            eepromWriteByte(address,waveform[m]>>8);
            eepromWriteByte(address+1,waveform[m] & 255);
            address+=2;
          }
          //Convert the received marks into integers and set them as the pulse parameters
          for (int k = 0; k<NUMPARAM; k++){
            waveform[k] = 0;
            for (int i = 0; i <16; i++){
              waveform[k] = waveform[k]<<1 | marks[7+i + k*16]>225;
            }
          }
          //Save the freshly updated pulse parameters to the designated parameter block (addresses 2-9) so they can be recalled when cerebro is turned off between sessions
          for (byte m = 0; m<NUMPARAM; m++){
            eepromWriteByte(2*m+2,waveform[m]>>8);
            eepromWriteByte(2*m+3,waveform[m] & 255);
          }
        }
        else if (pulsePairIndex==7 && marks[0]<falseThresh && marks[1]<falseThresh && marks[2]>trueThresh  && marks[3]<falseThresh  && marks[4]>trueThresh && marks[5]>trueThresh && marks[6]<falseThresh){
          calibrateMode = true;
        }
        // else{ //We received a message that we don't understand
        //   mySerial.println(pulsePairIndex);
        // }
        return pulsePairIndex;
      }
    }
    while (! (IR_inputReg & (1<<IR_pin))) {   // while the pin is low, count how long it is low by adding to markLength
      markLength++;
      delayMicroseconds(irResolution);
    }
    marks[pulsePairIndex] = markLength;
    pulsePairIndex++;
  }
}

void recordEvent(byte letter){
  unsigned long temp = millis();
  eepromWriteByte(address , letter);
  eepromWriteByte(address + 1, temp >>16 & 255);
  eepromWriteByte(address + 2, temp >> 8 & 255 );
  eepromWriteByte(address + 3, temp & 255);
  address+=4;
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

byte eepromReadByte( unsigned int writeAddress ) {  //"Random Read" read operation from FT24C64A Two-Wire Serial EEPROM datasheet
  //dummy write instruction
  TinyWireM.beginTransmission(0x50);
  TinyWireM.write(writeAddress >> 8);  // MSB
  TinyWireM.write(writeAddress & 255); // LSB
  TinyWireM.endTransmission();
  //"Current Address Read" read operation from datasheet
  TinyWireM.requestFrom(0x50,1);
  delay(10);
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
      mySerial.print(stamp);
      mySerial.print(",");
      mySerial.print(F("trigger\r"));
      k+=3;
    }
    else if (char(eepromReadByte(k))=='A'){
      stamp = first<<16|second<<8|third;
      mySerial.print(stamp);
      mySerial.print(",");
      mySerial.print(F("abort\r"));
      k+=3;
    }
    else if (char(eepromReadByte(k))=='C'){
      stamp = first<<16|second<<8|third;
      mySerial.print(stamp);
      mySerial.print(",");
      mySerial.print(F("continue\r"));
      k+=3;
    }
    else if (char(eepromReadByte(k))=='P'){
      unsigned int param1;
      for (int i = 0; i<NUMPARAM; i++){
        param1 = eepromReadByte(k+1+2*i)<<8;
        strcpy_P(buffer, (char*)pgm_read_word(&(parameterLabels[i])));    //Necessary casts and dereferencing
        mySerial.print(buffer);
        mySerial.print(word(param1|eepromReadByte(k+2*(i+1))));
        mySerial.print('\r');
      }
      k+=NUMPARAM*2;
    }
    else{
      mySerial.println(F("error"));
    }
  }
}

void printEEPROM(){
  mySerial.print(F("Ver,"));
  mySerial.print(version);
  mySerial.print('\r');
  unsigned int endingAddress = (eepromReadByte(0)<<8|eepromReadByte(1));
  readAddresses(12,endingAddress);   //print the list of events
  for (int i = 0; i<NUMPARAM; i++){
    strcpy_P(buffer, (char*)pgm_read_word(&(parameterLabels[i])));
    mySerial.print(buffer);
    mySerial.print(waveform[i]);
    mySerial.print('\r');
  }
  // if((BTN_inputReg & (1<<BTN_pin))){ //button is still being held even after the session events have been printed
  //   mySerial.println("Remaining Memory Contents:");
  //   readAddresses(endingAddress,8100); //print the remaining contents
  // }
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

void sendDAC(int value) {
  LATCH_outputReg &= ~(1<<LATCH_pin); //latch low selects the chip
  myShift(48);                        //Write to and Update (Power up) DAC Register command (page 13, table 1 of LTC2630-12 datasheet)
  myShift(value>>4);                  //shift high byte
  myShift(value<<4 & 255);            //shift low byte
  LATCH_outputReg |= (1<<LATCH_pin);  //latch high de-selects the chip
}

bool laserOFF(){
  LATCH_outputReg &= ~(1<<LATCH_pin); //latch low
  myShift(64);                        //Power down command (page 13, table 1 of LTC2630-12 datasheet)
  myShift(0);
  myShift(0);
  LATCH_outputReg |= (1<<LATCH_pin);  //latch high
  #ifndef MCUBE
  if (DAClevel==4095){
    isMaxed = true;
  }
  DAClevel = 0;
  #endif
  // DUNCE_outputReg &= ~(1<<DUNCE_pin); //Dunce ouput LOW
  return false;
}
