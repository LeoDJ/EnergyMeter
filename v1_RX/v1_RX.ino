#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

//Receiver Unit

#include <Wire.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
const uint8_t ledPin = 8;
RF24 radio(9,10); //Set up NRF24L01 on SPI pins + 9 & 10

const byte address[6] = "1EMtr";
const uint16_t rotsPerKwh = 375, timeout = 5000;

unsigned long curMils, lastData = 0, lastTrigTime = 0;
bool sigLostPrinted = false;
float watts=0;

struct data {
  unsigned long trigTime = 0;
  bool isKeepAlive = false;
} payload;

void setup()
{
  Serial.begin(115200);
  Serial.println(">> EnergyMeter v1_RX <<");
  lcd.begin(4,20);
  lcd.print("> EnergyMeter v1_RX<"); 
  radio.begin();
  radio.openReadingPipe(1, address); 
  radio.startListening(); 
}

void loop()
{
  curMils = millis();
  if(radio.available())
  {
    lcd.setCursor(19,0);
    lcd.print(".");
    lastData = curMils;
    
    //no need to filter on keepalive, because data only gets calculated, when new data comes in
    lastTrigTime = payload.trigTime; //save the lastTrig time from last received data
    
    while(radio.available())
    {
      radio.read(&payload, sizeof(payload));
    }
    if(!payload.isKeepAlive)
    {
      calcData();
      showData();
    }
  }
  if(curMils - lastData > timeout && !sigLostPrinted)
  {
    sigLostPrinted = true;
    lcd.clear();
    lcd.print("Signal lost");
  }
}

void calcData()
{
  float trigTimeDiff = payload.trigTime-lastTrigTime;
  watts = 3600000000 / (trigTimeDiff * (float)rotsPerKwh);
}

void showData()
{
  sigLostPrinted = false;
  lcd.clear();
  //lcd.print(String(payload.trigTime)); //test
  lcd.print("MomVerbr: ");
  lcd.print(watts);
  int tmp = 10 + String(watts).length();
  lcd.setCursor(tmp,0);
  lcd.print("W");
}

