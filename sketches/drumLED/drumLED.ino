/*

   drumLED.ino
   ===========

   bkinky lights from the arduino
   version 2.1

   modes
   -----

   see misc/modes.csv

   inputs
   ------

   - button
   - microphone
   - potentiometer

   outputs
   -------

   - LCD
   - LEDs

   hardware
   --------

   see misc/hardware.csv

 */


#include <avr/pgmspace.h>
#include "FastLED.h"
#include <Wire.h>
//#include <LCD.h>
//#include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>


// ========
//  DEFINES
// ========


/*
 * _VERSION_
 * release version of code
 */

#define _VERSION_ "2.1"

// FIXME: document all defines

// set to non-zero for serial output
#define _DEBUG_ 1

// hardware config
#define BUTTON_PIN 4
#define MIC_PIN A3
#define POT_PIN A0
#define DISPLAY_POINT_PIN 9
#define DISPLAY_START_PIN 6
#define LED_PIN 3
#define LCD_BACKLIGHT_PIN 10

// must be < 256
// FIXME: remove this limit
#define LED_NUM 148

// both between 0-255
// 'value' == brightness
#define DEFAULT_VALUE 64
#define DEFAULT_SATURATION 255

// LCD range: 0 - 255
#define LCD_BRIGHTNESS_ON 128
#define LCD_BRIGHTNESS_DIM 1

// break between modes: in milliseconds
#define MODE_CHANGE_DELAY 256

// LEDs off if no input within limit
#define MILLIS_UNTIL_OFF 4096

// longest time between iterations of LED changes
#define MAX_MILLIS_ITERATION 1024

// first mode
#define INIT_MODE 0

// max mode number
#define MAX_MODE 7


// ================
// GLOBAL VARIABLES
// ================

// button
// ------

// keep track of button state
bool bPressedG = false;

// this is initial mode
int8_t bModeG = INIT_MODE;

// to cycle through modes
int8_t bModulusG = MAX_MODE + 1;

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

// microphone is on
bool mListeningG = false;

// potentiometer
// -------------

// top level of pot (can be changed)
int pMaxPotLevelG = 672;

// led
// ---

// number of LEDs in attached strip
const uint8_t ledNumG = LED_NUM;

// default HSV value (brightness), max 255
uint8_t ledDefaultValueG = DEFAULT_VALUE;

// default HSV saturation, max 255
uint8_t ledDefaultSaturationG = DEFAULT_SATURATION;

// fastLED data structure
CRGB ledsG[ledNumG];

// lcd
// ---

// brightness setting when on
uint8_t lcdBrightnessOnG = LCD_BRIGHTNESS_ON;

// brightness setting when dimmed
uint8_t lcdBrightnessDimG = LCD_BRIGHTNESS_DIM;

/*
//progress bar character for brightness
byte lcdProgressBarG[8] = {
B11111,
B11111,
B11111,
B11111,
B11111,
B11111,
B11111,
};
 */

// LCD data structure
// 0x27 is the I2C bus address for an unmodified backpack
//LiquidCrystal  lcd(2,1,0,4,5,6,7);
LiquidCrystal_I2C  lcd(0x27,2,1,0,4,5,6,7);

// history
// -------

// whether to update regardless of pot change
bool alwaysUpdateG = false;

// current mode
uint8_t savedModeG = INIT_MODE;

// saved on scale from 0 to 1
float savedPotG;

// to allow for flexible mode timing
unsigned long savedTargetMillisG = 0;

// programatically hue manipulation
uint8_t savedHueG = 0;

// programatic manipulation of individual LEDs
uint8_t savedLedG = 0;

// programatically responding to audio changes
unsigned long savedLastSampleMillisG;

// multipurpose mode LED mirror
uint8_t savedParamsG[ledNumG];

// display
static char modeNameG[17] = "";

// how many of possible variables were stored
int eMaxStoredG;

// debugging
// ---------

// initialized from #define in setup()
bool debugG = true;


// =====
// SETUP
// =====

void setup() {
    // set debugging global var
    if(_DEBUG_ == 0) {
        debugG = false;
    }
    else {
        Serial.begin(9600);
    }

    pinMode(BUTTON_PIN, INPUT);
    pinMode(MIC_PIN, INPUT);
    pinMode(POT_PIN, INPUT);
    pinMode(LCD_BACKLIGHT_PIN, OUTPUT);

    FastLED.addLeds<NEOPIXEL, LED_PIN>(ledsG, ledNumG);
    initEEPROM();
    initLCD();
    if(debugG) {
        Serial.println("init() completed");
    }
}

void initEEPROM() {
    // tmp = updateNeeded
    int tmp = EEPROM.read(0);
    if(tmp) {
        // reset updateNeeded value
        EEPROM.write(0, 0);
        // save current values
        EEPROM.write(1, ledDefaultValueG);
        EEPROM.write(2, ledDefaultSaturationG);
        EEPROM.write(3, lcdBrightnessOnG);
        EEPROM.write(4, lcdBrightnessDimG);
        EEPROM.write(5, debugG);
        if(debugG) {
            Serial.println("all values written to EEPROM");
        }
    }
    else {
        ledDefaultValueG = EEPROM.read(1);
        ledDefaultSaturationG = EEPROM.read(2);
        lcdBrightnessOnG = EEPROM.read(3);
        lcdBrightnessDimG = EEPROM.read(4);
        debugG = EEPROM.read(5);
        if(debugG) {
            Serial.println("all values read from EEPROM");
        }
    }
    if(debugG) {
        for(int i = 0; i < 6; i++) {
            tmp = EEPROM.read(i);
            Serial.print(i);
            Serial.print(". ");
            Serial.println(tmp);
        }
    }
}

void initLCD() {

    lcd.init();

    // from example code
    // Print a message to the LCD.
    lcd.backlight();
    lcd.setCursor(3,0);
    lcd.print("Hello, world!");
    lcd.setCursor(2,1);
    lcd.print("Ywrobot Arduino!");
    lcd.setCursor(0,2);
    lcd.print("Arduino LCM IIC 2004");
    lcd.setCursor(2,3);
    lcd.print("Power By Ec-yuan!");

    /*
     * from dom
     analogWrite(LCD_BACKLIGHT_PIN,lcdBrightnessOnG);
     lcd.begin (16,2); // for 16 x 2 LCD module
     lcd.setBacklightPin(3,POSITIVE);
     lcd.setBacklight(HIGH);
     lcd.clear();
     lcd.createChar(0, lcdProgressBarG);
     */
}


// ====
// LOOP
// ====

void loop() {
    uint8_t mode = buttonGetValue();
    float pot = potentiometerScaled();
    bool updateMode = initLoop(mode, pot);
    if(updateMode) {
        modeSwitch(mode, pot);
        if(debugG) {
            Serial.print("Mode:");  // only print this when things change
            Serial.println(mode);
        }
    }
}

int buttonGetValue() {
    bool pressed = buttonWasPressed();
    if(pressed) {
        uint8_t currentButtonValue = bModeG;
        currentButtonValue += 1;
        bModeG = currentButtonValue % bModulusG;
    }
    return bModeG;
}

bool buttonWasPressed() {
    bool pressedState = false;
    uint8_t currentState = digitalRead(BUTTON_PIN);
    if(bPressedG && currentState == 0) {
        bPressedG = false;
        pressedState = true;
    }
    else if(!bPressedG && currentState == 1) {
        bPressedG = true;
    }
    return pressedState;
}

// scaled from 0 - 1
float potentiometerScaled() {
    int potLevel = analogRead(POT_PIN);
    if(potLevel > pMaxPotLevelG) {
        pMaxPotLevelG = potLevel + 1;
    }
    float ratio = (float) potLevel / (float) pMaxPotLevelG;

    // do not return 0
    // multiplying by 0 will break things
    if(ratio < 0.01) {
        ratio = 0.01;
    }
    return ratio;
}

bool initLoop(uint8_t md, float pt) {
    bool updateMode = false;
    if(md != savedModeG) {
        updateMode = true;
        savedModeG = md;
        // only init on mode change
        initMode(md, pt);
        if(debugG) {
            Serial.print("mode not matching");
        }
    }
    else if(pt != savedPotG) {
        updateMode = true;
        savedPotG = p;
        if(debugG) {
            Serial.print("pot not matching");
        }
    }
    else if(alwaysUpdateG) {
        updateMode = true;
        if(debugG) {
            Serial.print("alwaysUpdateG true");
        }
    }

    if(debugG){
        Serial.println(": initLoop()");
    }
    return updateMode;
}

void initMode(uint8_t m, float p) {
    // initialize variables
    // FIXME: should any of these go in EEPROM
    savedHueG = 0;
    savedLedG = 0;

    // fire
    if(m == 4) {
        // Add entropy to random number generator; we use a lot of it.
        random16_add_entropy(random());
    }
    // color/volume history
    else if(m == 6) {
        for(uint8_t i = 0; i < ledNumG; i++) {
            savedParamsG[i] = 0;
        }
    }
    // vu
    else if(m == 7) {
        // 85 / 255 == 120 / 360
        // 85 in 255 value hue value space
        // is the same as 120 in 360 degree hue (color) wheel
        float ratio = (float) 85 / (float) ledNumG;
        if(ratio < 1.0) {
            vuUnderSavedParams(ratio);
        }
        else {
            vuOverSavedParams(ratio);
        }
    }

    // a breath before switching modes
    dark();
    delay(MODE_CHANGE_DELAY);
}

void modeSwitch(uint8_t md, float pt) {

    /*
     * this is some LCD initialization code
     * maybe this should go initLCD()

     if (md ==-1)
     {
     savedModeG = md;
     printModeInfo(-1);  // after turning on - we loop here until somoene presses the mode button
     while (-1 == buttonGetValue());
     }

     */

    // FIXME: this is for mode7, should be put elsewhere
    int vuLevel = 0;

    switch(md)
    {
        case 0:
            mode0(pt);
            /*
             * these are in mode now
             setModeName("Color sweep");
             showModeIsListening(false);
             */
            break;
        case 1:
            mode1(pt);
            break;
        case 2:
            mode2(pt);
            break;
        case 3:
            mode3(pt);
            break;
        case 4:
            mode4(pt);
            break;
        case 5:
            mode5(pt);
            break;
        case 6:
            mode6(pt);
            break;
        case 7:
            mode7(pt);
            vuLevel=map(readPeakToPeak()*1024, 0, 2.2*1024, 0, 17);
            //LCD_BarGraph(vuLevel);
            break;
        default:
            modeFail(md);
    }
}

// FIXME: start external library functions
// collect functions into logical groups
// document collections

/*
 * this was in the main loop
 * needs to be put somewhere where all display modes will use it

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
 */

void setModeName(const char *string)
{
    strcpy(modeNameG, string);
}
/*
void printModeInfo(const char *string)
{
    strcpy(modeNameG, string);
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
        if (!modeNameG[0])
        {
            lcd.print(F("Mode :"));  lcd.print(mode);
        }
        else
        {
            lcd.print(modeNameG);
        }
        lcd.setCursor (0,1);        // go to start of 2nd line
        if (mListeningG)
            lcd.print(F("Mic on "));
        else
            lcd.print(F("Mic off"));
        lastMode = mode;

        Serial.print("New mode:");
        Serial.println(lastMode);
        writeEEProm(savedModeG, lastMode);
    }



}
*/

void showModeIsListening(bool listening)
{
    mListeningG = listening;
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
        // FIXME: returned mode because arduino syntax demands it
        // maybe there is something better to return
        return mode;
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
        analogWrite(LCD_BACKLIGHT_PIN,lcdBrightnessDimG);  // PWM values from 0 to 255 (0% – 100%
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
            analogWrite(LCD_BACKLIGHT_PIN,lcdBrightnessOnG);  // PWM values from 0 to 255 (0% – 100%
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


// eeprom
// ------

int readEEProm(uint8_t location)
{
    return EEPROM.read(location);
}

void writeEEProm(uint8_t location, uint8_t value )
{
    EEPROM.write(location, value);
}



// ==============
// MODE FUNCTIONS
// ==============

void dark()
{
    for(uint8_t i = 0; i < ledNumG; i++)
    {
        ledsG[i] = CHSV(0, 0, 0);
    }
    FastLED.show();
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
    static byte heat[ledNumG];
    bool gReverseDirection = false;

    // Step 1.  Cool down every cell a little
    for( uint8_t i = 0; i < ledNumG; i++) {
        heat[i] = qsub8( heat[i],  random8(0, ((fireCooling * 10) / ledNumG) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( uint8_t k= ledNumG - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < fireSparking ) {
        uint8_t y = random8(7);
        heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( uint8_t j = 0; j < ledNumG; j++) {
        CRGB color = HeatColor( heat[j]);
        uint8_t pixelnumber;
        if( gReverseDirection ) {
            pixelnumber = (ledNumG - 1) - j;
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

// arg r is 85 / ledNumG
void vuUnderSavedParams(float r)
{
    float rawMultiplier = 1.0 / r;
    int truncatedMultiplier = (int) rawMultiplier;
    int truncatedTotal = 85 * truncatedMultiplier;
    int remainder = ledNumG - truncatedTotal;
    uint8_t currentHue = 0;
    uint8_t truncatedCount = 0;
    bool remainderAdded = false;

    // best for hues to start from red (0)
    // and go toward green (85)
    // so we populate array "backwards"
    // since 0 is value at end
    for(int i = ledNumG - 1; i > -1; i--)
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

// arg r is 85 / ledNumG
void vuOverSavedParams(float r)
{
    int hueAddend = (int) r;
    int truncatedTotal = hueAddend * ledNumG;
    int remainder = 85 - truncatedTotal;
    uint8_t currentHue = 0;
    for(int i = ledNumG - 1; i > -1; i--)
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
    for(uint8_t i = 0; i < ledNumG; i++)
    {
        ledsG[i] = CHSV(hue, ledDefaultSaturationG, ledDefaultValueG);
    }
    FastLED.show();
}


// rainbow strip
// pot controls rotation speed
void mode1(float p)
{
    alwaysUpdateG = true;

    // divide color space among given LEDs
    // this code will only work with ledNumG < 255
    // since ledNumG is uint8_t this is not a realistic worry ...
    // FIXME: write needed code so ledNumG can be any value
    float fHueStep = 255.0 / (float) ledNumG;
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
        for(uint8_t i = 0; i < ledNumG; i++)
        {
            uint8_t tmp = hue + hueStep;
            hue = tmp % 255;
            ledsG[i] = CHSV(hue, ledDefaultSaturationG, ledDefaultValueG);
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
        for(uint8_t i = 0; i < ledNumG; i++)
        {
            ledsG[i] = CHSV(hue, ledDefaultSaturationG, ledDefaultValueG);
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

    uint8_t lastIndex = ledNumG - 1;
    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG)
    {
        unsigned long delayMillis = computeNextIteration(p);
        savedTargetMillisG = currentMillis + delayMillis;
        uint8_t tmp = savedHueG + 1;
        uint8_t hue = tmp % 255;
        tmp = savedLedG + 1;
        uint8_t onLed = tmp % ledNumG;
        uint8_t offLed = onLed - 1;
        if(onLed == 0)
        {
            offLed = lastIndex;
        }
        savedLedG = onLed;
        savedHueG = hue;
        ledsG[onLed] = CHSV(hue, ledDefaultSaturationG, ledDefaultValueG);
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
    FastLED.setBrightness(ledDefaultValueG);
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
        for(uint8_t i = 0; i < ledNumG; i++)
        {
            ledsG[i] = CHSV(hue, ledDefaultSaturationG, ledDefaultValueG);
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
        for(int i = ledNumG - 2; i > -1; i--)
        {
            j = i + 1;
            savedParamsG[j] = savedParamsG[i];
        }
        savedParamsG[0] = hue;

        // display
        for(uint8_t i = 0; i < ledNumG; i++)
        {
            if(savedParamsG[i] > 0)
            {
                ledsG[i] = CHSV(savedParamsG[i], ledDefaultSaturationG, ledDefaultValueG);
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
        // volts scaled to ledNumG
        float numerator = cookedVolts * (float) ledNumG;
        float rawSeparator = numerator / mMaxVoltsG;
        int separatorIndex = round(rawSeparator);

        // for timekeeping
        savedLastSampleMillisG = millis();

        // only LEDs below separatorIndex are lit
        // others are dark
        for(uint8_t i = 0; i < ledNumG; i++)
        {
            if(i > separatorIndex)
            {
                ledsG[i] = CHSV(0, 0, 0);
            }
            else
            {
                ledsG[i] = CHSV(savedParamsG[i], ledDefaultSaturationG, ledDefaultValueG);
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

