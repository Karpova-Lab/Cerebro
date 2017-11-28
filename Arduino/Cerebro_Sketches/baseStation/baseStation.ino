/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MIT License

Copyright (c) 2015-2017 Andy S. Lustig

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

//Documentation for this project can be found at https://karpova-lab.github.io/cerebro/

#include <Radio.h>  //https://github.com/LowPowerLab/RFM69
#include <SPI.h>

const uint8_t VERSION = 46;

const int16_t LED = 13;
const int16_t TRIGGER_PIN = 5;
const int16_t STOP_PIN = 6;

Radio radio(8,7); //slave select pin, interrupt pin
WaveformData newWaveform;
WaveformData currentWaveform;
DiodePowers diodePwrs;
IntegerPayload integerMessage;
Info cerebroInfo;
Feedback diodeStats;

uint32_t  valsFromParse[5];
uint32_t  startTime = 0;
uint32_t  spamFilter = 0;
uint32_t  triggerClock = 0;
uint16_t  msgCount = 0;

void setup() {
  Serial1.begin(57600);
  delay(10);
  pinMode(LED, OUTPUT);
  radio.radioSetup(1,false); //nodeID, autopower on;
  pinMode(TRIGGER_PIN,INPUT);
  pinMode(STOP_PIN,INPUT);
  startTime = millis();  
  newSession();
}

void loop() {
  ////////////Receive Message From Bcontrol
  if (digitalRead(TRIGGER_PIN)) {
    while(digitalRead(TRIGGER_PIN)){
      //wait until signal goes low
    } 
    stopCommandReceived();
  }
  //if we read a high signal on pin 6, send a stop command to cerebro
  if (digitalRead(STOP_PIN)) {
    while(digitalRead(STOP_PIN)){
      //wait until signal goes low
    }       
    triggerCommandReceived();
  }

  ///////////Receive Message From Xavier//////////////////
  if (Serial1.available()){
    char msg = Serial1.read();
    if (msg=='W'){  //parse data then send new waveform
      parseData();
      sendWaveformUpdate();
    }
    else if (msg=='S' || msg == 'L' || msg == 'R' ||  msg == 'l' ||  msg == 'r'){ //parse data then send radio message with integer
      parseData();
      delay(500);
      sendMsgAndVal(msg,valsFromParse[0]);
    }
    else if (msg=='T'){ 
      triggerCommandReceived();
    }
    else if (msg=='A'){ 
      stopCommandReceived();
    }
    else if(msg=='N'){
      newSession();
    }
    else if (msg!='\n'){ 
      relayMsg(msg);  
    }
  }

  ///////////Receive Message From Cerebro///////
  if (radio.receiveDone()){
    switch (radio.DATALEN) {
      case sizeof(cerebroInfo):
        if (radio.ACKRequested()){
          radio.sendACK();
        }
        cerebroInfo = *(Info*)radio.DATA;
        printCerebroInfo();
        break;
      case sizeof(diodePwrs):
        if (radio.ACKRequested()){
          radio.sendACK();
        }
        diodePwrs = *(DiodePowers*)radio.DATA;
        printDiodePowers();
        break;
      case sizeof(currentWaveform):
        if (radio.ACKRequested()){
          radio.sendACK();
        }
        currentWaveform = *(WaveformData*)radio.DATA;
        if (currentWaveform.trainDur>0){
          spamFilter = currentWaveform.startDelay + currentWaveform.trainDur;
        }
        else{
          spamFilter = currentWaveform.startDelay + currentWaveform.onTime;
        }
        printWaveform(currentWaveform,true);
        break;
      case sizeof(diodeStats):
        if (radio.ACKRequested()){
          radio.sendACK();
        }
        diodeStats = *(Feedback*)radio.DATA;  
        printDiodeStats();   
        break;
      case  sizeof(integerMessage):
        if (radio.ACKRequested()){
          radio.sendACK();
        }
        integerMessage = *(IntegerPayload*)radio.DATA;  
        if(integerMessage.variable == 'B'){
          printBattery(integerMessage.msgCount,integerMessage.value);
        }
        else if (integerMessage.variable == 'M'){
          Serial1.print("Total Missed,");Serial1.print(integerMessage.value);newline();       
        }
        else if (integerMessage.variable =='m'){
          Serial1.print("Missed Message Index,");Serial1.print(integerMessage.value);newline();      
        }
        else if (integerMessage.variable == 'Y'){
          Serial1.print("Cerebro turned on and connected,");Serial1.print(integerMessage.value);//print time it took to startup and send connection message.
        }
        break;        
    }
  }
}

void parseData(){
  char msgData[30] = "";
  Serial1.readBytesUntil('\n',msgData,30);
  char* msgPointer;
  msgPointer = strtok(msgData,",");
  char i = 0;
  while (msgPointer!=NULL){
    valsFromParse[i] = atol(msgPointer);   
    msgPointer = strtok(NULL,",");
    i++;
  }
}

void sendMsgAndVal(char msg,unsigned int val){
  integerMessage.variable = msg;
  integerMessage.value = val;
  triggerClock = millis();      
  Serial1.print("\nSending '"); Serial1.print(msg);Serial1.print("' ") ;Serial1.print(integerMessage.value);Serial1.print("...");
  if (radio.sendWithRetry(CEREBRO, (const void*)(&integerMessage), sizeof(integerMessage))){
    Serial1.print("data received");newline();    
  }
  else{
    Serial1.print("*X&data send fail");newline();
  }
}

void newSession(){
  startTime = millis();
  msgCount = 0;
  Serial1.print("\n*BaseOn&Base Version,");Serial1.print(VERSION);newline();
  char msg = 'N';
  if (radio.sendWithRetry(CEREBRO, &msg, 1, 3)){ 
    // Serial1.print("Connected!");
  }
  else{      
    Serial1.print("*X&Error communicating with Cerebro\n\n");
  }
}

void relayMsg(char msg){
  if (radio.sendWithRetry(CEREBRO, &msg, 1, 0)){  // 0 = only 1 attempt, no retries
  }
  else{
    Serial1.print("*X&");
    Serial1.print(currentTime());comma();Serial1.print("Tried Sending ''");Serial1.print(msg);
    Serial1.print("'', ACK not received");newline();
  }
}    

void printCerebroInfo(){
  Serial1.print("*Cerebro Info~");
  Serial1.print(cerebroInfo.firmware);tilda();Serial1.print(cerebroInfo.serialNumber); 
  Serial1.print("&");
  Serial1.print("Cerebro Version,");Serial1.print(cerebroInfo.firmware);newline();  
  Serial1.print("Serial Number,");Serial1.print(cerebroInfo.serialNumber);newline();
}

void printDiodePowers(){
  Serial1.print(currentTime());comma();
  Serial1.print("[");Serial1.print(diodePwrs.msgCount);Serial1.print("]");
  Serial1.print("*Diode Powers~");
  Serial1.print(diodePwrs.lSetPoint);tilda();Serial1.print(diodePwrs.rSetPoint); 
  Serial1.print("&");
  Serial1.print(",Diode Powers,");Serial1.print(diodePwrs.lSetPoint);dash();Serial1.print(diodePwrs.rSetPoint);newline();   
}

void printBattery(uint8_t batteryMsgCount, uint8_t batteryStatus){
  Serial1.print(currentTime());comma();
  Serial1.print("[");Serial1.print(batteryMsgCount);Serial1.print("]");  
  Serial1.print("*Battery~");
  Serial1.print(batteryStatus);
  Serial1.print("&");
  Serial1.print(",Battery,");Serial1.print(batteryStatus);newline(); 
}

void printWaveform(WaveformData wave, bool response){
  Serial1.print("*Waveform~");
  Serial1.print(wave.startDelay);tilda();
  Serial1.print(wave.onTime);tilda();
  Serial1.print(wave.offTime);tilda();
  Serial1.print(wave.trainDur);tilda();
  Serial1.print(wave.rampDur);
  Serial1.print("&");
  Serial1.print(currentTime());comma();
    if(response){
      Serial1.print("[");Serial1.print(wave.msgCount);Serial1.print(']');
    }
    else{
      Serial1.print(msgCount);
    }
  Serial1.print(",Waveform,");
  Serial1.print(wave.startDelay);dash();
  Serial1.print(wave.onTime);dash();
  Serial1.print(wave.offTime);dash();
  Serial1.print(wave.trainDur);dash();
  Serial1.print(wave.rampDur);newline();
}

void printDiodeStats(){
  Serial1.print(currentTime());
  Serial1.print(",[");Serial1.print(diodeStats.msgCount);Serial1.print("],");
  Serial1.print("Feedback,");
  Serial1.print(diodePwrs.lSetPoint);dash();
  Serial1.print(diodeStats.leftFBK);dash();
  Serial1.print(diodeStats.leftDAC);dash();
  Serial1.print(diodePwrs.rSetPoint);dash();               
  Serial1.print(diodeStats.rightFBK);dash();
  Serial1.print(diodeStats.rightDAC);newline();
  if (diodeStats.leftDAC>3000){
    Serial1.print("Warning: Left DAC value of ");Serial1.print(diodeStats.leftDAC);Serial1.print(" is suspicously high\n");
  }
  if (diodeStats.rightDAC>3000){
    Serial1.print("Warning: Right DAC value of ");Serial1.print(diodeStats.rightDAC);Serial1.print(" is suspicously high\n");
  }
}

void sendWaveformUpdate(){
  newWaveform.startDelay = valsFromParse[0];
  newWaveform.onTime = valsFromParse[1];
  newWaveform.offTime = valsFromParse[2];
  newWaveform.trainDur = valsFromParse[3];
  newWaveform.rampDur = valsFromParse[4];
  newWaveform.msgCount = msgCount++;
  printWaveform(newWaveform,false);  
  if (radio.sendWithRetry(CEREBRO, (const void*)(&newWaveform), sizeof(newWaveform))){
    currentWaveform = newWaveform;
  }
  else{
    Serial1.print("*X&Waveform Update Failed\n");
  }
}

void triggerCommandReceived(){
  uint32_t  tSinceTrigger = millis() - triggerClock;
  if (tSinceTrigger>spamFilter){
    msgCount++;    
    integerMessage.variable = 'T';    
    integerMessage.value = msgCount;
    Serial1.print(currentTime());comma();Serial1.print(msgCount);comma();Serial1.print("Trigger");newline();
    radio.send(CEREBRO, (const void*)(&integerMessage), sizeof(integerMessage));
    triggerClock = millis();
  }
  else{
    msgCount++;        
    integerMessage.variable = 'C';    
    integerMessage.value = msgCount;
    Serial1.print(currentTime());comma();Serial1.print(msgCount);comma();Serial1.print("Continue");newline();
    radio.send(CEREBRO, (const void*)(&integerMessage), sizeof(integerMessage));
    triggerClock = millis();    
  }
}

void stopCommandReceived(){
  uint32_t  tSinceTrigger = millis() - triggerClock;
  if (tSinceTrigger<spamFilter){
    msgCount++;      
    integerMessage.variable = 'A';    
    integerMessage.value = msgCount;
    Serial1.print(currentTime());comma();Serial1.print(msgCount);comma();Serial1.print("Abort");newline();
    radio.send(CEREBRO, (const void*)(&integerMessage), sizeof(integerMessage));
    triggerClock = -spamFilter; //prevents back to back stop signals from being sent
  }
  // else{
  //   Serial1.print(currentTime());comma();comma();Serial1.print("Ignore Bcontrol,");Serial1.print(spamFilter);newline();    
  // }
}

uint32_t  currentTime(){
  return millis()-startTime;
}

void comma(){
  Serial1.print(",");
}

void tilda(){
  Serial1.print("~");
}

void dash(){
  Serial1.print("-");
}

void newline(){
  Serial1.print("\n");
}