/*
  E-Bike Controller Software

  There are 3 Buttons:
    GO: Starts the Motor
    UP: Increases the motor current if GO (or continous mode) is Only
    DOWN: Decreases the motor current

  Continous GO:
  In order to be able to release the GO button and still power the motor,
  the Continous GO mode was implemented:
  Start Continous GO:
  Press the GO Button + Press the UP Button (GO must be first)
  Stop Continous GO:
  Press the GO Button
*/

#include <Arduino.h>
#include <VescUart.h>

#include <Adafruit_GFX.h>  // Include core graphics library
#include <Adafruit_ST7735.h>  // Include Adafruit_ST7735 library to drive the display

#define BTN_UP_PIN 2
#define BTN_DOWN_PIN 12
#define BTN_MENU_PIN 8 //Does no longer exist...
#define BTN_GO_PIN 3

#define TFT_CS     7
#define TFT_RST    9  // You can also connect this to the Arduino reset in which case, set this #define pin to -1!
#define TFT_DC     10

VescUart vesc;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

bool BTN_UP, BTN_DOWN,BTN_MENU,BTN_GO; 
bool BTN_UP_RISING, BTN_DOWN_RISING,BTN_MENU_RISING,BTN_GO_RISING; 
bool CONTINOUS_GO = false;

bool dutyCycleMode = true; //True = dutyCycleMode, False = currentMode

float throttle = 0;
float throttle_increment = 0.1;
float throttle_max = 1;
float throttle_min = 0;
float throttle_currentMode_max_current_amps = 50;

float batteryVoltage = 0;

#define SPEED_LIMIT 25
bool speed_limit_enabled = true;
float speed_kmh = 0;

#define nofBatPercentageLookups 21
#define batPercentageIncrements 100/(nofBatPercentageLookups-1)
#define maxBatPercentage 100
#define minBatPercentage 0
const float batVoltageLookup[nofBatPercentageLookups] = {33.1,36.8,36.9,37.1,37.4,37.6,37.7,37.8,37.9,38.1,
                                                         38.2,38.4,38.8,39.2,39.5,39.8,40.0,40.5,40.9,41.3,41.8};

void setupStaticLCDitems()
{
  char stringBuf[10];

  //Power %  
  strcpy(stringBuf, "Power");
  tft.setCursor(25, 8);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(stringBuf);

  //U_Bat  
  strcpy(stringBuf, "U Bat");
  tft.setCursor(25, 50);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(stringBuf);

  //Amps %  
  strcpy(stringBuf, "Amps");
  tft.setCursor(105, 8);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(stringBuf);

  //U_Bat  
  strcpy(stringBuf, "% Bat");
  tft.setCursor(105, 50);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(stringBuf);

  //U_Bat  
  strcpy(stringBuf, "kmh");
  tft.setCursor(120, 110);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(2);
  tft.println(stringBuf);

}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode( BTN_UP_PIN, INPUT_PULLUP );
  pinMode( BTN_DOWN_PIN, INPUT_PULLUP );
  pinMode( BTN_MENU_PIN, INPUT_PULLUP );
  pinMode( BTN_GO_PIN, INPUT_PULLUP );
  Serial.begin(115200);

  tft.initR(INITR_BLACKTAB);  // Initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_BLACK);  // Fill screen with black
  tft.setRotation(1);
  setupStaticLCDitems();

  vesc.setSerialPort(&Serial);
  
}

void readButtons()
{
    static bool last_UP_state,last_DOWN_state,last_MENU_state,last_GO_state;
    last_UP_state = BTN_UP;
    last_DOWN_state = BTN_DOWN;
    last_MENU_state = BTN_MENU;
    last_GO_state = BTN_GO;

    BTN_UP = !digitalRead(BTN_UP_PIN);
    BTN_DOWN = !digitalRead(BTN_DOWN_PIN);
    BTN_MENU = !digitalRead(BTN_MENU_PIN);
    BTN_GO = !digitalRead(BTN_GO_PIN);

    if(!last_UP_state && BTN_UP){BTN_UP_RISING = true;} else{ BTN_UP_RISING = false;}
    if(!last_DOWN_state && BTN_DOWN){BTN_DOWN_RISING = true;} else{ BTN_DOWN_RISING = false;}
    if(!last_MENU_state && BTN_MENU){BTN_MENU_RISING = true;} else{ BTN_MENU_RISING = false;}
    if(!last_GO_state && BTN_GO){BTN_GO_RISING = true;} else{ BTN_GO_RISING = false;}
}

/*
  Continous GO:
  In order to be able to release the GO button and still power the motor,
  the Continous GO mode was implemented:
  Start Continous GO:
  Press the GO Button + Press the UP Button (GO must be first)
  Stop Continous GO:
  Press the GO Button
*/
void setContinousGoIfBtnCombo()
{
    if(BTN_GO & BTN_UP_RISING)
    {
        CONTINOUS_GO = true;
        BTN_UP_RISING = false;
    }
    else if(BTN_GO_RISING)
    {
        CONTINOUS_GO = false;
    }
}

void toggleDutyCycleMode(bool doToggle)
{
  if(doToggle)
  {
    dutyCycleMode = !dutyCycleMode;
  }
}

void setThrottleAccordingButtons()
{
  if(BTN_UP_RISING)
  {
    if(throttle + throttle_increment <= throttle_max+0.01)
    {
    throttle +=throttle_increment;
    }      
  }
  else if(BTN_DOWN_RISING)
  {
    if(throttle - throttle_increment >= throttle_min)
    {
    throttle -=throttle_increment;
    }      
  }
}

void printState()
{
  Serial.print(" UP_EDGE:");
  Serial.print(BTN_UP_RISING);
  Serial.print(" DOWN_EDGE:");
  Serial.print(BTN_DOWN_RISING);
  Serial.print(" MENU_EDGE:");
  Serial.print(BTN_MENU_RISING);
  Serial.print(" GO:");
  Serial.print(BTN_GO);

  Serial.print(" Throttle:");
  Serial.print(throttle);
  Serial.print("\n");
}



int interpolateBetweenPercentageIncrements(float batVoltage, float lowerLookup, float upperLookup)
{
    float lookupDiff = upperLookup-lowerLookup;
    float batVoltDiff = batVoltage-lowerLookup;

    return (int)(((float)batPercentageIncrements/lookupDiff)*batVoltDiff);
}

int getBatPercentage(float batVoltage)
{

    for(int i= 0; i<nofBatPercentageLookups; i++)
    {
        if(i==(nofBatPercentageLookups-1))
        {
            return maxBatPercentage;
        }
        else if(batVoltage>=batVoltageLookup[i] && batVoltage>=batVoltageLookup[i+1])
        {
            continue;
        }
        else if(batVoltage>=batVoltageLookup[i] && batVoltage<=batVoltageLookup[i+1])
        {
            return (i*batPercentageIncrements)+interpolateBetweenPercentageIncrements(batVoltage,batVoltageLookup[i],batVoltageLookup[i+1]);
        }
        else if(batVoltage<=batVoltageLookup[0])
        {
            return minBatPercentage;
        }
    }
}

/*
LCD Screen: 1.8 inch st7735r spi 128x160 

--------------
|0 -->160(x)  |
||            |
|v 128(y)     |
--------------
*/
void printOnLCD()
{
  char stringBuf[10];

  //Throttle  
  if(dutyCycleMode)
  {
    dtostrf(throttle*100, 3, 0, stringBuf);
    strcat(stringBuf, "%");
    tft.setCursor(5, 20);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setTextSize(3);
    tft.println(stringBuf);
  }
  else
  {
    dtostrf(throttle*throttle_currentMode_max_current_amps, 3, 0, stringBuf);
    strcat(stringBuf, "A");
    tft.setCursor(5, 20);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setTextSize(3);
    tft.println(stringBuf);
  }

  //Battery Voltage
  dtostrf(vesc.data.inpVoltage, 3, 0, stringBuf);
  strcat(stringBuf, "V");
  tft.setCursor(5, 60);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(3);
  tft.println(stringBuf);

  //Battery Percent
  dtostrf(getBatPercentage(vesc.data.inpVoltage), 3, 0, stringBuf);
  strcat(stringBuf, "%");
  tft.setCursor(85, 60);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(3);
  tft.println(stringBuf);

  //Amerage
  dtostrf(vesc.data.avgMotorCurrent, 3, 0, stringBuf);
  strcat(stringBuf, "A");
  tft.setCursor(85, 20);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(3);
  tft.println(stringBuf);

  //Speed
  //RPM 2145[umfangRag] /197.92 [Umfang Motor] = 10.838
  //MotorWindungen = 9 
  //2.145m/(10.838*60)=0.0032985790 * 3.6 / 9  = 0.001319431629 --> RPM TO KMH

  //Airwheel motor:
  //D=0.325m, 15 pole pairs
  // kmh = (rpm*pi*D*60)/(15*1000)=rpm*(3.14159265359×0.325×60)/(15×1000)=rpm*0.004084070450
  speed_kmh = vesc.data.rpm*(double)0.004084070450;
  dtostrf(speed_kmh, 3, 0, stringBuf);
  tft.setCursor(30, 90);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(4);
  tft.println(stringBuf);

  //indicateGo
  if(BTN_GO)
  {
      tft.drawRect(0, 0, 160, 128, ST7735_CYAN);  // Draw rectangle (x,y,width,height,color)
  }
  else if(CONTINOUS_GO)
  {
      tft.drawRect(0, 0, 160, 128, ST7735_RED);  // Draw rectangle (x,y,width,height,color)
  }
  else
  {
      tft.drawRect(0, 0, 160, 128, ST77XX_BLACK);  // Draw rectangle (x,y,width,height,color)
  }
  
}

void errorBlink()
{    
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW); 
  delay(200);
}

void writeThrottleToVescIfGoPressed()
{
  if((speed_kmh < SPEED_LIMIT  || !speed_limit_enabled) && (BTN_GO || CONTINOUS_GO))
  {
      if(dutyCycleMode)
      {
          // vesc.setDuty(throttle);
          vesc.nunchuck.valueY = (throttle * 127) + 127;
          vesc.setNunchuckValues();
      }
      else
      {
          vesc.setCurrent(throttle*throttle_currentMode_max_current_amps);
      }     
  }
  else
  {
    if(dutyCycleMode)
    {
        // vesc.setDuty(throttle);
        vesc.nunchuck.valueY = 127;
        vesc.setNunchuckValues();
    }
    else
    {
        vesc.setDuty(0);
    }   
  }    
}

/*
  Press all buttons (UP DOWN and GO) for 5 seconds to toggle 
  the speed limit (25kmh)
  If the speedlimit is now enabled, a red dot is displayed as long as
  the buttons are pressed
  If the speedlimit is now disabled, a green dot is displayed as long as
  the buttons are pressed
*/
void disableSpeedLimitIfBtnCombo(void)
{
    static int timeAllButtonsPressed_ms = 0;
    static int timeStampNotAllButtonsPressed_ms = 0;
    static bool toggled = false;

    if(BTN_GO && BTN_UP && BTN_DOWN)
    {
      timeAllButtonsPressed_ms = millis() - timeStampNotAllButtonsPressed_ms;   
    }
    else
    {
      timeStampNotAllButtonsPressed_ms = millis();
      timeAllButtonsPressed_ms = 0;
      toggled = false;
      tft.fillCircle(10, 108, 5, ST7735_BLACK);
      return;
    }  

    if(timeAllButtonsPressed_ms > 5000)
    {
      if(toggled == false)
      {
        speed_limit_enabled = !speed_limit_enabled;  
        toggled = true;   
      }

      if(speed_limit_enabled)
      {
        tft.fillCircle(10, 108, 5, ST7735_RED);
      }
      else
      {
        tft.fillCircle(10, 108, 5, ST7735_GREEN);
      }
    }
}

void loop() {
  readButtons();
  setContinousGoIfBtnCombo();
  //toggleDutyCycleMode(BTN_MENU_RISING); //Menu Button does no longer exist
  setThrottleAccordingButtons();
  if ( vesc.getVescValues() ) 
  {
    writeThrottleToVescIfGoPressed();
  }
  else
  {
    vesc.setDuty(0);
    errorBlink();
  }
  //printState(); //Only For debugging!
  printOnLCD();
  disableSpeedLimitIfBtnCombo();
  //delay(5);              
}