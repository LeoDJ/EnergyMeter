#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

//Metering + Transmission Unit

const uint8_t ledPin = 8, irLedPin = 2, irSensPin = A1;
RF24 radio(9,10); //Set up NRF24L01 on SPI pins + 9 & 10

const byte address[6] = "1EMtr";
const uint8_t  thresh = 3;
const uint16_t trigTimeout = 1000;

int16_t valAct;
bool wasTriggered;
uint64_t curMils, prevMils0=0, lastTrigMils=0, lastKAMils=0;
int16_t n=0, n2=0; //n=avgValCount
float avgVal=0, avgVal2=0;

struct data {
  unsigned long trigTime = 0;
  bool isKeepAlive = false;
} payload;

void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin, OUTPUT);
  pinMode(irLedPin, OUTPUT);
  Serial.begin(115200);
  Serial.println(">> EnergyMeter v1_TX <<");
  radio.begin();
  radio.openWritingPipe(address);
}

void loop() {
  /*digitalWrite(ledPin, HIGH); RF test stuff
  payload.pings = micros();
  radio.write(&payload, sizeof(payload));
  digitalWrite(ledPin, LOW);
  delay(200);*/

  curMils = millis(); //read millis only once, because of performance
  digitalWrite(irLedPin, HIGH);
  delayMicroseconds(128);
  valAct = analogRead(irSensPin); //read active reflection value
  digitalWrite(irLedPin, LOW);

  buildAvg(valAct);
  checkTrig(valAct, curMils);
  
  if (curMils - lastKAMils > 1000) //send a keepAlive packet every second
  {
    lastKAMils = curMils;
    sendKeepAlive();
  }
}

void sendKeepAlive() //send the keepAlive packet
{
  payload.isKeepAlive = true;
  radio.write(&payload, sizeof(payload));
  payload.isKeepAlive = false;
}

void trigger(uint64_t trigMils)
{
  digitalWrite(ledPin, HIGH); 
  payload.trigTime = trigMils;
  radio.write(&payload, sizeof(payload));
  digitalWrite(ledPin, LOW);
}

void buildAvg(int16_t val) //build average ambient sensor value
{

  if(n == 0) //initialize avg
    avgVal = val;
  else
  {
    avgVal = ((avgVal * (n-1)) / n) + ((float)val / n); //running average calculation
  }
  ++n;
  if(n > 10000) n = 100; //reset average weight 
}


void checkTrig(int val, uint64_t curMils) //build smaller average for the current sensor value and check for trigger condition
{
  if(n2 == 0) //initialize avg2
    avgVal2 = val;
  else
  {
    avgVal2 = ((avgVal2 * (n2-1)) / n2) + ((float)val / n2);
  }

  if(n2 >= 5) //if Data is valid
  {
    if(avgVal - avgVal2 > thresh) //when current reading is smaller than average reading
    {
      if((curMils - lastTrigMils > trigTimeout) && !wasTriggered) //trigger error prevention
      {
        trigger(curMils); //trigger
        lastTrigMils = curMils; //save this time as last triggered time
        wasTriggered = true; //error prevention
      }
    }
    else
      wasTriggered = false; //when reading is back to normal reset error prevention variable
  }

  ++n2;
  if(n2 > 20) n2 = 5; //reset average weight 
}
