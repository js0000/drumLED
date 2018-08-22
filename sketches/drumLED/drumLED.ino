/*

    drumLED.ino
    bkinky lights from the arduino

    version 1.0

    8 modes

    see misc/modes.csv

    inputs
    - button
    - microphone
    - potentiometer

    outputs
    - display
    - led

   s see misc/hardware.csv
*/

// FIXME: comment document
// use comment blocks to mark of big parts of the code
// see http://www.sphinx-doc.org/en/master/usage/quickstart.html

#define _VERSION_ "18.04.01"
#include <avr/pgmspace.h>
#include "FastLED.h" 
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// FIXME: this goes under LCD in some way ...
LiquidCrystal_I2C  lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the I2C bus address for an unmodified backpack

// =======
// DEFINES
// =======

#define BUTTON_PIN 4
#define MIC_PIN A3
#define POT_PIN A0
#define DISPLAY_POINT_PIN 9
#define DISPLAY_START_PIN 6
#define LED_PIN 3

// must be < 256
#define LED_NUM 148

// both must be between 0-255
// 'value' == brightness
#define DEFAULT_VALUE 64
#define DEFAULT_SATURATION 255

// break between modes: in milliseconds
#define MODE_CHANGE_DELAY 256

// LEDs off if no input within limit
#define MILLIS_UNTIL_OFF 4096

// longest time between iterations of LED changes
#define MAX_MILLIS_ITERATION 1024

// FIXME: this goes in hardware config EEPROM
#define EEsize 1024
#define LAST_MODE 0

// =======
// GLOBALS
// =======

// button
// ------
bool bPressedG = false;

// this is initial mode
int8_t bModeG = -1;
int8_t firstMode = -1;

// microphone
// ----------
// this is changeable (can increase)
// allows for more coverage of color spectrum
float mMaxVoltsG = 0.6;

// anything below this is considered silence
const float mVoltFloorG = 0.05;

// to better fill out the color space
// these may change during operation
int mMaxRawHueG = 192;
int mMinRawHueG = 32;

// potentiometer
// -------------
int pMaxPotLevelG = 672;
short lastPotLevelReading = 0;

// FIXME: this can be deleted
// display
// ------
const uint8_t dDigitArraySizeG = 7;

// led
// ---
const uint8_t lNumG = LED_NUM;
const uint8_t lDefaultValueG = DEFAULT_VALUE;
const uint8_t lDefaultSaturationG = DEFAULT_SATURATION;

// fastLED data structure
CRGB ledsG[lNumG];

// FIXME: LCD  ... ?
//progress bar character for brightness
byte pBar[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
};

// FIXME: eeprom variables go here
// history
// -------
bool alwaysUpdateG = false;
uint8_t savedModeG = 0;

// saved on scale from 0 to 1
float savedPotG;
unsigned long savedTargetMillisG = 0;
uint8_t savedHueG = 0;
uint8_t savedLedG = 0;
unsigned long savedLastSampleMillisG;
uint8_t savedParamsG[lNumG];

// FIXME: this is LCD
//LCD
#define backlight_pin10 10
static char modeName[17] = "";
bool bListening = false;
//LCD parameters
#define LCD_ON_Brightness 128
#define LCD_Dim_Brightness 1

 
// =======
// ARDUINO
// =======

// FIXME: loop function should go here

void setup()
{
    pinMode(BUTTON_PIN, INPUT);
    pinMode(MIC_PIN, INPUT);
    pinMode(POT_PIN, INPUT);

    pinMode(backlight_pin10, OUTPUT);         // sets pin10 as output
    analogWrite(backlight_pin10,LCD_ON_Brightness);  // PWM values from 0 to 255 (0% – 100% 

    // FIXME: this can be deleted
    // <= i instead of < i
    // due to inclusion of point pin
    // which is not accounted for in dDigitArraySizeG
    for(uint8_t i = 0; i <= dDigitArraySizeG; i++)
    {
        uint8_t displayPin = DISPLAY_START_PIN + i;
        pinMode(displayPin, OUTPUT);
    }

    // FIXME: LCD initialization into subroutine
  // activate LCD module
  lcd.begin (16,2); // for 16 x 2 LCD module
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.createChar(0, pBar);
    
    FastLED.addLeds<NEOPIXEL, LED_PIN>(ledsG, lNumG);

    Serial.begin(9600);

    // FIXME: eepROM initializaiton in it's own subroutine
    uint8_t eepromLastMode = readEEProm(LAST_MODE);
    if (eepromLastMode != 255)
    {
      Serial.println(" Some mode was stored.");
      Serial.println(eepromLastMode);
      firstMode = eepromLastMode;
      
    } else
    {
      Serial.println(" No mode was stored yet.");
    }
    strcpy(modeName, "");

}

// FIXME: start external library functions
// collect functions into logical groups
// document collections

void setModeName(const char *string)
{  
   strcpy(modeName, string);
}

void printModeInfo(const char *string)
{
     strcpy(modeName, string);
     printModeInfo(0);
}

// FIXME: a debugging routine
void printModeInfo(int mode)
{
  static int lastMode=mode;

  
//  Serial.print(F("printModeInfo ")); Serial.print(mode);
    
  if (mode == -1)
  {  // in startup the mode is set to -1
    lcd.clear();
    lcd.print(F("drumLED "));
    lcd.print(_VERSION_);
    lcd.setCursor (0,1);        // go to start of 2nd line
    lcd.print(F("Push button"));
    return;
  }


  if (lastMode != mode)
  {
   lcd.clear();
   lcd.home (); // set cursor to 0,0
   lcd.setCursor (0,0);        // go to start of 2nd line
   if (!modeName[0])
   {
      lcd.print(F("Mode :"));  lcd.print(mode);
   }
   else
   {
      lcd.print(modeName);
   }     
   lcd.setCursor (0,1);        // go to start of 2nd line
   if (bListening)
     lcd.print(F("Mic on "));
   else 
     lcd.print(F("Mic off"));
   lastMode = mode;

   Serial.print("New mode:");
   Serial.println(lastMode);
   writeEEProm(LAST_MODE, lastMode);
  } 

  
  
}

void showModeIsListening(bool listening)
{
 bListening = listening;
}

// FIXME: this is a utility mode called by several methods
void LCD_BarGraph(short level)
{
  int i;
  
  //prints the progress bar
  for (i=0; i<level; i++)
  {
    lcd.setCursor(i, 1);   
    lcd.write(byte(0));  
  }
  for (i=level; i<16; i++)
  {
    lcd.setCursor(i, 1);   
    lcd.write(" ");  
  }


}

// FIXME: this can be a mode
void loopBarGraph()
{  //this is loop() function that just displays the bargraph controlled by the potentiometer on the lcd display
   int level = 0;         // progress bar
   int i;
  // clears the LCD screen
   lcd.setCursor(0, 0);   
   lcd.write("This is a test");  
      
   level=map(potentiometerScaled()*1024, 0, 1024, 0, 17);
   LCD_BarGraph(level);
  
  // delays 750 ms
  delay(75);        


}

// FIXME: LCD maintenance
int dimDisplayIfControlsNotRecentlyTouched()
{
    
    int mode = buttonGetValue();
    float pot = potentiometerScaled();
    
      // variables used to keep track time since any control was touched.
    static int lastMode = mode;
    static float lastPot = pot;
    static unsigned long  millisSinceLastUpate = millis();
    static int lastSecond = 0;
    static bool dimmed=false;

    if (isButtonPressed())  // we don't dim if the button is down
    {   millisSinceLastUpate = millis();
        return;
    }
    int secondsPast=(millis()-millisSinceLastUpate)/1000;
        
    if (lastSecond != secondsPast)  // if a second has elapsed...
    {
       lastSecond = secondsPast;
    }

    if (lastMode != mode)  // check to see if button has been pressed... reset time if so.
    { 
      millisSinceLastUpate = millis();      
    }

    if (lastMode != -1 && lastMode == mode && secondsPast > 5 && !dimmed)
    {  // if we have not just turned on, and our mode has not changed, and we are not dimmed then dim
      dimmed = true;
      analogWrite(backlight_pin10,LCD_Dim_Brightness);  // PWM values from 0 to 255 (0% – 100% 
    } 
    else // otherwise if we are dimmed, mode has changed or pot has changed then wake up
    if( dimmed && (lastMode != mode  ||
        abs(lastPot - pot) > .05 ))
    {  
      if (lastMode != mode) 
      {// after waking up we don't want mode to change so check if it was button press that woke us up
         mode = decrementButtonValue();
      } 
      dimmed = false;
      analogWrite(backlight_pin10,LCD_ON_Brightness);  // PWM values from 0 to 255 (0% – 100% 
      lastMode = mode;
      lastPot = pot;
      millisSinceLastUpate = millis();
    }   

    lastMode = mode;
    
    return mode;
}

// FIXME: i don't think long press is worth doing
bool isModeButtonHeldDownFor3Secs()
{  // has the user held down the mode button 
    static unsigned long  millisSinceLastUpate = millis();
    static int lastSecond = 0;
    
    int secondsPast=(millis()-millisSinceLastUpate)/1000;

    if (isButtonPressed())
    {    
       if (lastSecond != secondsPast)  // if a second has elapsed...
       {
        Serial.print("mode button pressed for: "); Serial.println(secondsPast);
          lastSecond = secondsPast;
          if (secondsPast > 2)
          {
            //We are about to return true - we only do that once after button down for 5 secs
             millisSinceLastUpate = millis(); // ensure we reset timer
            return true;
          }
       }   
    } else
    {
       millisSinceLastUpate = millis(); 
       if (lastSecond != secondsPast)  // if a second has elapsed...
       {
           Serial.print("time passing: "); Serial.println(secondsPast);
           lastSecond = secondsPast;
       }
    }

   return false;
//     Serial.print("millisSinceLastUpate :"); Serial.println(millisSinceLastUpate);
}

// FIXME: there should only be one loop, with everything in it
// this is a debugging mode
void LED_DisplayTheMode()
{
    int mode = dimDisplayIfControlsNotRecentlyTouched();
    float pot = potentiometerScaled();    
    int vuLevel = 0;

    if (mode ==-1)
    {
      savedModeG = mode;
      printModeInfo(-1);  // after turning on - we loop here until somoene presses the mode button
      while (-1 == buttonGetValue());
    }

    
    boolean updateMode = false;
    if(mode != savedModeG)
    {
        updateMode = true;
        modeInit(mode);
    }
    else if(pot != savedPotG)
    {
        updateMode = true;
    }
    else if(alwaysUpdateG)
    {
        updateMode = true;
    }

    if(updateMode)
    {
        savedModeG = mode;
        savedPotG = pot;
        setModeName("");
        switch(mode)
        {
            case 0:
                mode0(pot);
                setModeName("Color sweep");
                showModeIsListening(false);
            break;
            case 1:
                mode1(pot);
                setModeName("Rainbow cycle");
                showModeIsListening(false);
                break;
            case 2:
                setModeName("Slow Hue change");
                mode2(pot);
                showModeIsListening(false);
                break;
            case 3:
                setModeName("Chasing dot");
                mode3(pot);
                showModeIsListening(false);
                break;
            case 4:
                setModeName("Fire!!");
                showModeIsListening(false);
                mode4(pot);
                break;
            case 5:
                setModeName("Pulsed light");
                showModeIsListening(true);
                mode5(pot);
                break;
            case 6:
                mode6(pot);
                setModeName("Chased light");
                showModeIsListening(true);
                break;
            case 7:
                setModeName("VU Meter");
                showModeIsListening(true);
                mode7(pot);
                vuLevel=map(readPeakToPeak()*1024, 0, 2.2*1024, 0, 17);
                LCD_BarGraph(vuLevel);
                break;            
            case -1:
            case 255:  // this is -1... just fall out of case statement
                break;
            default:
                modeFail(mode);
        }
        printModeInfo(mode);
    }

}

// FIXME: loop is way too complex, needs to be abstracted
void loop()
{

   static bool displayMode = true;  // we are either in display mode or settings mode

   if (displayMode)
   {
    static int lastDisplayMode = buttonGetValue();
    if (buttonGetValue()!= lastDisplayMode) {
       Serial.print("Mode:");  // only print this when things change
       Serial.println(buttonGetValue());
    }
    lastDisplayMode = buttonGetValue(); 
    LED_DisplayTheMode();
   }
   else
   {
       lcd.setCursor (0,0);        // go to start of 2nd line
       printModeInfo("Settings...");
       lcd.setCursor (0,1);        // go to start of 2nd line
       lcd.print(F("               "));

   }

   if (isModeButtonHeldDownFor3Secs())
   {
      displayMode = !displayMode;
      if (displayMode)
         decrementButtonValue();
   }
   
   
}


// ======
// HARDWARE
// ======

// button
// ------
int setButtonMode(uint8_t newValue)
{  // we use this to preset the mode state for the button control
    bModeG = newValue;
    return bModeG;
}
int decrementButtonValue()
{  // we use this to preset the mode state for the button control
    int currentButtonValue = bModeG;
    currentButtonValue -= 1;
    if (currentButtonValue < 0)
      currentButtonValue = 7;
    bModeG = currentButtonValue;
    return bModeG;
}

int buttonGetValue()
{
    bool pressed = buttonWasPressed();
    if(pressed)
    {
        if (firstMode == -1)
        {  // there was no first mode in eeprom memory.  this might be first time turned on.
          uint8_t currentButtonValue = bModeG;
          currentButtonValue += 1;
          bModeG = currentButtonValue % 8;
        } else
        {  // after turning on and firstMode read from memory is non-1 means that we have a mode to restore to.
          bModeG= firstMode;
          firstMode=-1;
        }
    }
    return bModeG;
}

bool buttonWasPressed()
{
    bool pressedState = false;
    uint8_t currentState = digitalRead(BUTTON_PIN);
    if(bPressedG && currentState == 0)
    {
        bPressedG = false;
        pressedState = true;
    }
    else if(!bPressedG && currentState == 1)
    {
        bPressedG = true;
    }
    return pressedState;
}

bool isButtonPressed()
{
    bool currentState = !digitalRead(BUTTON_PIN);
    
    return currentState;
}


// microphone
// ----------
// pretty much stolen from adafruit
// https://learn.adafruit.com/adafruit-microphone-amplifier-breakout/measuring-sound-levels
float readPeakToPeak()
{
    // Sample window width in mS (50 mS = 20Hz)
    unsigned long sampleWindow = 100;

    unsigned long startMillis = millis();  // Start of sample window
    unsigned int peakToPeak = 0;   // peak-to-peak level
    unsigned int signalMax = 0;
    unsigned int signalMin = 1024;
    unsigned int sample;

    // collect data for sampleWindow
    while(millis() - startMillis < sampleWindow)
    {
        sample = analogRead(MIC_PIN);
        // toss out spurious readings
        if(sample < 1024)
        {
            // save just the max levels
            if(sample > signalMax)
            {
                signalMax = sample;
            }
            // save just the min levels
            else if(sample < signalMin)
            {
                signalMin = sample;
            }
        }
    }
    // max - min = peak-peak amplitude
    peakToPeak = signalMax - signalMin;

    // convert to volts
    // float volts = (peakToPeak * 5.0) / 1024;
    float volts = (peakToPeak * 3.3) / 1024;
    return volts;
}

// potentiometer
// -------------
// scaled from 0 - 1
float potentiometerScaled()
{
    lastPotLevelReading = analogRead(POT_PIN);

    int potLevel = lastPotLevelReading;
    
    if(potLevel > pMaxPotLevelG)
    {
        pMaxPotLevelG = potLevel + 1;
    }
    float ratio = (float) potLevel / (float) pMaxPotLevelG;

    // do not return 0
    // multiplying by 0 will break things
    if(ratio < 0.01)
    {
        ratio = 0.01;
    }
    return ratio;
}


// ==============
// MODE FUNCTIONS
// ==============

void dark()
{
    for(uint8_t i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(0, 0, 0);
    }
    FastLED.show();
}

void modeInit(uint8_t m)
{
    // initialize variables
    savedHueG = 0;
    savedLedG = 0;
    savedPotG = potentiometerScaled();

    // fire
    if(m == 4)
    {
        // Add entropy to random number generator; we use a lot of it.
        random16_add_entropy(random());
    }
    // color/volume history
    else if(m == 6)
    {
        for(uint8_t i = 0; i < lNumG; i++)
        {
            savedParamsG[i] = 0;
        }
    }
    // vu
    else if(m == 7)
    {
        // 85 / 255 == 120 / 360
        // 85 in 255 value hue value space
        // is the same as 120 in 360 degree hue (color) wheel
        float ratio = (float) 85 / (float) lNumG;
        if(ratio < 1.0)
        {
            vuUnderSavedParams(ratio);
        }
        else
        {
            vuOverSavedParams(ratio);
        }
    }

    // a breath before switching modes
    dark();
    delay(MODE_CHANGE_DELAY);
}


// stolen from https://github.com/FastLED/FastLED/blob/master/examples/Fire2012/Fire2012.ino

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
////
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation,
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking.
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).


void Fire2012(uint8_t fs)
{
    // There are two main parameters you can play with to control the look and
    // feel of your fire: COOLING (used in step 1 above), and SPARKING (used
    // in step 3 above).

    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 50, suggested range 20-100
    uint8_t fireCooling = 55;

    uint8_t fireSparking = fs;

    // Array of temperature readings at each simulation cell
    static byte heat[lNumG];
    bool gReverseDirection = false;

    // Step 1.  Cool down every cell a little
    for( uint8_t i = 0; i < lNumG; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((fireCooling * 10) / lNumG) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( uint8_t k= lNumG - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < fireSparking ) {
      uint8_t y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( uint8_t j = 0; j < lNumG; j++) {
      CRGB color = HeatColor( heat[j]);
      uint8_t pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (lNumG - 1) - j;
      } else {
        pixelnumber = j;
      }
      ledsG[pixelnumber] = color;
    }
}


// returns zero if below mVoltFloorG
int voltsToHue(float v)
{

    int computedHue = 0;
    if(v > mVoltFloorG)
    {
        // recalibrate mMaxVoltsG if necessary
        if(v > mMaxVoltsG)
        {
            mMaxVoltsG = v + 0.01;
        }

        // recalibrate rawHues if necessary
        if(computedHue > mMaxRawHueG)
        {
            mMaxRawHueG = (computedHue + 1);
            if(mMaxRawHueG > 255)
            {
                mMaxRawHueG = 255;
            }
        }
        else if(computedHue < mMinRawHueG)
        {
            mMinRawHueG = (computedHue - 1);
            if(mMinRawHueG < 0)
            {
                mMinRawHueG = 0;
            }
        }
        float hueRange = mMaxRawHueG - mMinRawHueG;
        float numerator = v * hueRange;
        float voltRange = mMaxVoltsG - mVoltFloorG;
        float ratio = numerator / voltRange;
        computedHue = round(ratio);
    }
    return computedHue;
}

// prepares raw hue to be put on HSV color scheme
// compares against previous value
//   to determine which "side" of hue cirle
//   to place value upon
uint8_t scaleHueAll(uint8_t rh)
{
    boolean louder = true;
    if(savedHueG > rh)
    {
        louder = false;
    }

    // make loudest 0
    // easier to scale
    uint8_t invertedHue = 255 - rh;

    // scale to 128
    uint8_t computedHue = invertedHue / 2;

    // 0-127 louder, 128-255 quieter
    if(!louder)
    {
        // on a circle, not a line
        computedHue = 256 - computedHue;
    }

    return computedHue;
}


int offsetHue(uint8_t offset, uint8_t hue, uint8_t maxHue)
{
    int offsetHue = offset + hue;
    uint8_t offsetValue = offsetHue % maxHue;
    return offsetValue;
}

unsigned long computeNextIteration(float p)
{
    float fRawDelay = MAX_MILLIS_ITERATION * p;
    int rawDelay = round(fRawDelay);

    // flip
    unsigned long nextIteration = MAX_MILLIS_ITERATION - rawDelay;

    return nextIteration;
}

// arg r is 85 / lNumG
void vuUnderSavedParams(float r)
{
    float rawMultiplier = 1.0 / r;
    int truncatedMultiplier = (int) rawMultiplier;
    int truncatedTotal = 85 * truncatedMultiplier;
    int remainder = lNumG - truncatedTotal;
    uint8_t currentHue = 0;
    uint8_t truncatedCount = 0;
    bool remainderAdded = false;

    // best for hues to start from red (0)
    // and go toward green (85)
    // so we populate array "backwards"
    // since 0 is value at end
    for(int i = lNumG - 1; i > -1; i--)
    {
        savedParamsG[i] = currentHue;
        truncatedCount++;
        if(truncatedCount >= truncatedMultiplier)
        {
            if(remainder > 0)
            {
                if(!remainderAdded)
                {
                    remainder -= 1;
                    remainderAdded = true;
                }
                else
                {
                    currentHue += 1;
                    truncatedCount = 0;
                    remainderAdded = false;
                }
            }
            else
            {
                currentHue += 1;
                truncatedCount = 0;
                remainderAdded = true;
            }
        }
    }
}

// arg r is 85 / lNumG
void vuOverSavedParams(float r)
{
    int hueAddend = (int) r;
    int truncatedTotal = hueAddend * lNumG;
    int remainder = 85 - truncatedTotal;
    uint8_t currentHue = 0;
    for(int i = lNumG - 1; i > -1; i--)
    {
        savedParamsG[i] = currentHue;
        currentHue += hueAddend;
        if(remainder > 0)
        {
            currentHue += 1;
            remainder -= 1;
        }
    }
}


// ======
// MODES
// ======

// solid color
// pot changes hue
void mode0(float p)
{
    alwaysUpdateG = false;

    float rawHue = 255.0 * p;
    float flippedHue = 255.0 - rawHue;
    uint8_t hue = round(flippedHue);
    for(uint8_t i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
    }
    FastLED.show();
}


// rainbow strip
// pot controls rotation speed
void mode1(float p)
{
    alwaysUpdateG = true;

    // divide color space among given LEDs
    // this code will only work with lNumG < 255
    // since lNumG is uint8_t this is not a realistic worry ...
    float fHueStep = 255.0 / (float) lNumG;
    if(fHueStep < 1.0)
    {
        fHueStep = 1.0;
    }
    uint8_t hueStep = round(fHueStep);

    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG){
        unsigned long delayMillis = computeNextIteration(p);
        savedTargetMillisG = currentMillis + delayMillis;
        uint8_t hue = savedHueG;
        for(uint8_t i = 0; i < lNumG; i++)
        {
            uint8_t tmp = hue + hueStep;
            hue = tmp % 255;
            ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
            if(i == 0)
            {
                 savedHueG = hue;
            }
        }
        FastLED.show();
    }
}

// rainbow all LEDs via time
// pot controls rate of hue change
void mode2(float p)
{
    alwaysUpdateG = true;

    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG){
        unsigned long delayMillis = computeNextIteration(p);
        savedTargetMillisG = currentMillis + delayMillis;
        int tmp = savedHueG + 1;
        uint8_t hue = tmp % 255;
        savedHueG = hue;
        for(uint8_t i = 0; i < lNumG; i++)
        {
            ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
        }
        FastLED.show();
    }
}

// rainbow singleton LED
//   single LED travels from start to end of strip
//   changing color for each LED
// pot controls speed
void mode3(float p)
{
    alwaysUpdateG = true;

    uint8_t lastIndex = lNumG - 1;
    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG)
    {
        unsigned long delayMillis = computeNextIteration(p);
        savedTargetMillisG = currentMillis + delayMillis;
        uint8_t tmp = savedHueG + 1;
        uint8_t hue = tmp % 255;
        tmp = savedLedG + 1;
        uint8_t onLed = tmp % lNumG;
        uint8_t offLed = onLed - 1;
        if(onLed == 0)
        {
            offLed = lastIndex;
        }
        savedLedG = onLed;
        savedHueG = hue;
        ledsG[onLed] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
        ledsG[offLed] = CHSV(hue, 0, 0);
        FastLED.show();
    }
}

// fire
// pot controls sparking
void mode4(float p)
{
    alwaysUpdateG = true;

    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    // lowering to 25
    float fsparking = 175.0 * p;
    uint8_t temp = round(fsparking);
    uint8_t sparking = 25 + temp;

    uint8_t framesPerSecond = 60;
    FastLED.setBrightness(lDefaultValueG);
    Fire2012(sparking); // run simulation frame
    FastLED.show(); // display this frame
    FastLED.delay(1000 / framesPerSecond);
}

// random-ish colors from mic volume
// pot controls mic peak level
void mode5(float p)
{
    alwaysUpdateG = true;

    uint8_t hue = savedHueG;

    float rawVolts = readPeakToPeak();
    float cookedVolts = rawVolts * p;
    int rawHue = voltsToHue(cookedVolts);

    if(rawHue != 0)
    {
        uint8_t scaledHue = scaleHueAll(rawHue);
        // 43/256 ~= 60/360
        hue = offsetHue(43, scaledHue, 255);
        savedLastSampleMillisG = millis();
    }
    else
    {
        // check timing
        unsigned long currentMillis = millis();
        unsigned long millisSinceLastSample = currentMillis - savedLastSampleMillisG;
        if(millisSinceLastSample > MILLIS_UNTIL_OFF)
        {
            dark();
        }
    }

    // only update LEDS if there is a change
    if(savedHueG != hue)
    {
        for(uint8_t i = 0; i < lNumG; i++)
        {
            ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
        }
        FastLED.show();

        // reset
        savedHueG = hue;
    }
}

// color/volume history pushed onto array
// pot controls mic peak level
void mode6(float p)
{
    alwaysUpdateG = true;

    uint8_t hue = savedHueG;

    float rawVolts = readPeakToPeak();
    float cookedVolts = rawVolts * p;
    uint8_t rawHue = voltsToHue(cookedVolts);

    if(rawHue != 0)
    {
        uint8_t scaledHue = scaleHueAll(rawHue);
        // 43/256 ~= 60/360
        hue = offsetHue(43, scaledHue, 255);
        savedLastSampleMillisG = millis();
    }
    else
    {
        // check timing
        unsigned long currentMillis = millis();
        unsigned long millisSinceLastSample = currentMillis - savedLastSampleMillisG;
        if(millisSinceLastSample > MILLIS_UNTIL_OFF)
        {
            dark();
        }
    }

    // only update LEDS if there is a change
    if(savedHueG != hue)
    {
        // update savedParamsG
        // starting with 2nd from top
        // leaving bottom for hue
        uint8_t j;
        // i is int not uint8_t because
        //    negative number comparison needs to work
        for(int i = lNumG - 2; i > -1; i--)
        {
            j = i + 1;
            savedParamsG[j] = savedParamsG[i];
        }
        savedParamsG[0] = hue;

        // display
        for(uint8_t i = 0; i < lNumG; i++)
        {
            if(savedParamsG[i] > 0)
            {
                ledsG[i] = CHSV(savedParamsG[i], lDefaultSaturationG, lDefaultValueG);
            }
            // blanks
            else
            {
                ledsG[i] = CHSV(0, 0, 0);
            }
        }
        FastLED.show();

        // reset
        savedHueG = hue;
    }
}

// VU meter
// pot controls mic peak level
void mode7(float p)
{
    alwaysUpdateG = true;

    // savedParamsG populated by vuOverSavedParamsG() or vuUnderSavedParamsG()

    float rawVolts = readPeakToPeak();
    float cookedVolts = rawVolts * p;

    if(cookedVolts <= mVoltFloorG)
    {
        // check timing
        unsigned long currentMillis = millis();
        unsigned long millisSinceLastSample = currentMillis - savedLastSampleMillisG;
        if(millisSinceLastSample > MILLIS_UNTIL_OFF)
        {
            dark();
        }
    }
    else
    {
        // volts scaled to lNumG
        float numerator = cookedVolts * (float) lNumG;
        float rawSeparator = numerator / mMaxVoltsG;
        int separatorIndex = round(rawSeparator);

        // for timekeeping
        savedLastSampleMillisG = millis();

        // only LEDs below separatorIndex are lit
        // others are dark
        for(uint8_t i = 0; i < lNumG; i++)
        {
            if(i > separatorIndex)
            {
                ledsG[i] = CHSV(0, 0, 0);
            }
            else
            {
                ledsG[i] = CHSV(savedParamsG[i], lDefaultSaturationG, lDefaultValueG);
            }
        }
        FastLED.show();
    }
}

void modeFail(uint8_t m)
{
    Serial.print(F("invalid mode passed in: "));
    Serial.println(m);
}

// FIXME: these should go in eeprom hardware section
int readEEProm(uint8_t location)
{
   return EEPROM.read(location);
}

void writeEEProm(uint8_t location, uint8_t value )
{
    EEPROM.write(location, value);
}


