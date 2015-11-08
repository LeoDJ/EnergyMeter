#include <FastIO.h>
#include <I2CIO.h>
#include <LCD.h>
#include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal_SR.h>
#include <LiquidCrystal_SR2W.h>
#include <LiquidCrystal_SR3W.h>

#define DEBUG 0

LiquidCrystal lcd(9, 8, 7, 6, 5, 4);

const uint8_t irLedPin = 2, irSensPin = A1, thresh = 3;
const uint16_t trigTimeout = 1000, rotsPerKwh = 375;


void setup() 
{
  lcd.begin(16,2);
  pinMode(irLedPin, OUTPUT);
  Serial.begin(115200);
  Serial.println(">> CNY70 Test <<");
}

int valAct, valPas;
bool wasTriggered;
float watts=0, kWh=0;

uint64_t curMils, prevMils0=0, lastTrigMils=0;

int n=0, n2=0; //n=avgValCount
float avgVal=0, avgVal2=0;

void loop() 
{
  curMils = millis(); //read millis only once, because of performance
  valPas = analogRead(irSensPin); //read passive reflection value
  digitalWrite(irLedPin, HIGH);
  delayMicroseconds(128);
  valAct = analogRead(irSensPin); //read active reflection value
  digitalWrite(irLedPin, LOW);
  delayMicroseconds(128);
  //Serial.println((String)valAct + " | " + (String)valPas);

  buildAvg(valAct);
  checkTrig(valAct, curMils);

  if(curMils - prevMils0 > 250) //update LCD every 0.25s
  {
    prevMils0 = curMils;
    printLcd();
  }
  //delay(25);
}

void printLcd()
{
  lcd.setCursor(0,0);
  lcd.clear();
  delay(1);
  #if DEBUG
    lcd.setCursor(0,0); //Print debug data to LCD
    lcd.print(avgVal);
    lcd.setCursor(8,0);
    lcd.print(avgVal2);
    lcd.setCursor(0,1);
    lcd.print(n);
    lcd.setCursor(5,1);
    lcd.print(valAct);
    lcd.setCursor(9,1);
  #else
    lcd.setCursor(0,0); //Print data to LCD
    lcd.print(watts);
    lcd.setCursor(8,0);
    lcd.print("W");
    lcd.setCursor(0,1);
    lcd.print(kWh);
    lcd.setCursor(8,1);
    lcd.print("kWh");
  #endif
}

void buildAvg(int val) //build average ambient sensor value
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

void trigger(uint64_t curMils)
{
  blinkLed();

  kWh += (1/(float)rotsPerKwh); //calculate data
  float trigTime = curMils-lastTrigMils;
  watts = 3600000000 / (trigTime * (float)rotsPerKwh);

  #if DEBUG
    lcd.print("TRIG!");
    Serial.print(avgVal); //print debug information
    Serial.print(", ");
    Serial.print(n);
    Serial.print("; ");
    Serial.print(avgVal2);
    Serial.print(", ");
    Serial.print(n2);
    Serial.print("; ");
    Serial.println(valAct);
  #endif
  
}

void checkTrig(int val, uint64_t curMils) //build smaller average vor current sensor value and check for trigger condition
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


void blinkLed()
{
  digitalWrite(13, HIGH);
  delay(10);
  digitalWrite(13, LOW);
}
/*
class avg
{
  private: 
    uint16_t n = 0;
    double avgVal;
  public:
    double val();
}*/