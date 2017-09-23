/*

    Drumβ (beta)
    skipped unicode char in file name

    This sketch is to gather all the code
    for drumLED into one giant file
    to make sure it works as expected

    Then it should be rewritten as library

    Also, not including adafruit lib as it is superfluous with fastLED

    inputs
    - button
    - microphone
    - potentiometer

    outputs
    - display
    - led

*/

#include "FastLED.h"


// =======
// DEFINES
// =======

#define BUTTON_PIN 4
#define MIC_PIN 2
#define POT_PIN A0
#define DISPLAY_POINT_PIN 9
#define DISPLAY_START_PIN 6
#define LED_PIN 3

// "rainbow" modes will not work if this is > 255
#define LED_NUM 64

// between 0-255
// 'value' == brightness
#define DEFAULT_VALUE 128
#define DEFAULT_SATURATION 255


// =======
// GLOBALS
// =======

// button
// ------
const int bPinG = BUTTON_PIN;
bool bPressedG = false;
int bValueG = 0;

// microphone
// ----------
const int mPinG = MIC_PIN;

// Sample window width in mS (50 mS = 20Hz)
const int mSampleWindowG = 200;

// anything below this is considered silence
const double mVoltFloorG = 0.05;

// this is changeable (can increase)
// allows for more coverage of color spectrum
double mMaxVoltsG = 0.6;

// potentiometer
// -------------
const int pPinG = POT_PIN;
// pMaxPotLevelG will not be needed with standardized hardware
int pMaxPotLevelG = 672;

// display
// ------
const int dPointPinG = DISPLAY_POINT_PIN;
const int dStartPinG = DISPLAY_START_PIN;
const int dDigitArraySizeG = 7;
const int dDigitMatrixG[16][dDigitArraySizeG] = {
    //0 1 2 3 4 5 6
    { 1,1,1,1,1,1,0 },  // = 0
    { 0,0,1,1,0,0,0 },  // = 1
    { 1,1,0,1,1,0,1 },  // = 2
    { 0,1,1,1,1,0,1 },  // = 3
    { 0,0,1,1,0,1,1 },  // = 4
    { 0,1,1,0,1,1,1 },  // = 5
    { 1,1,1,0,1,1,1 },  // = 6
    { 0,0,1,1,1,0,0 },  // = 7
    { 1,1,1,1,1,1,1 },  // = 8
    { 0,1,1,1,1,1,1 },  // = 9
    { 1,0,1,1,1,1,1 },  // = A
    { 1,1,1,0,0,1,1 },  // = b
    { 1,1,0,0,0,0,1 },  // = c
    { 1,1,1,1,0,0,1 },  // = d
    { 1,1,0,0,1,1,1 },  // = E
    { 1,0,0,0,1,1,1 }   // = F
};

/*
    dDigitMatrixG bit map

        4
    5       3
        6
    0       2
        1
*/

// led
// ---
const int lPinG = LED_PIN;
const int lNumG = LED_NUM;
uint8_t hueStepG;
const int lDefaultValueG = DEFAULT_VALUE;
const int lDefaultSaturationG = DEFAULT_SATURATION;

// fastLED data structure
CRGB ledsG[lNumG];

// history
// -------
bool alwaysUpdateG = false;
int previousModeG = 0;
float previousPotG;
int targetMillisG = 0;
uint8_t previousHueG = 0;

// =======
// ARDUINO
// =======

void setup()
{
    pinMode(bPinG, INPUT);
    pinMode(mPinG, INPUT);
    pinMode(pPinG, INPUT);

    // <= i instead of < i
    // due to inclusion of point pin
    // which is not accounted for in dDigitArraySizeG
    for(int i = 0; i <= dDigitArraySizeG; i++)
    {
        int displayPin = dStartPinG + i;
        pinMode(displayPin, OUTPUT);
    }

    FastLED.addLeds<NEOPIXEL, lPinG>(ledsG, lNumG);
    setupInit();

    // FIXME: remove this before prod
    Serial.begin(9600);
}

void loop()
{
    int mode = buttonGetValue();
    float pot = potentiometerScaledValue();

    boolean updateMode = false;
    if(mode != previousModeG)
    {
        updateMode = true;
    }
    else if(pot != previousPotG)
    {
        updateMode = true;
    }
    else if(alwaysUpdateG)
    {
        updateMode = true;
    }

    if(updateMode)
    {
        previousModeG = mode;
        previousPotG = pot;
        displayMode(mode);
        switch(mode)
        {
            case 0:
                mode0(pot);
                break;
            case 1:
                mode1(pot);
                break;
            case 2:
                mode2(pot);
                break;
            case 3:
                mode3(pot);
                break;
            case 4:
                mode4(pot);
                break;
            case 5:
                mode5(pot);
                break;
            case 6:
                mode6(pot);
                break;
            case 7:
                mode7(pot);
                break;
            case 8:
                mode8(pot);
                break;
            case 9:
                mode9(pot);
                break;
            case 10:
                modeA(pot);
                break;
            case 11:
                modeB(pot);
                break;
            case 12:
                modeC(pot);
                break;
            case 13:
                modeD(pot);
                break;
            case 14:
                modeE(pot);
                break;
            case 15:
                modeF(pot);
                break;
            default:
                modeFail(mode);
        }

        // FIXME: remove before prod
        // this just separates mode changes by blank line
        Serial.println("");
    }

    // FIXME: remove before prod
    // avoids data firehose effect
   //delay(1000);
}


// ======
// HARDWARE
// ======

// button
// ------
int buttonGetValue()
{
    bool pressed = buttonWasPressed();
    if(pressed)
    {
        int currentButtonValue = bValueG;
        currentButtonValue += 1;
        bValueG = currentButtonValue % 16;
    }
    return bValueG;
}

bool buttonWasPressed()
{
    bool pressedState = false;
    int currentState = digitalRead(bPinG);
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

// microphone
// ----------
// pretty much stolen from adafruit
// https://learn.adafruit.com/adafruit-microphone-amplifier-breakout/measuring-sound-levels
double readPeakToPeak()
{
    unsigned long startMillis = millis();  // Start of sample window
    unsigned int peakToPeak = 0;   // peak-to-peak level
    unsigned int signalMax = 0;
    unsigned int signalMin = 1024;
    unsigned int sample;

    // collect data for sampleWindow
    while(millis() - startMillis < mSampleWindowG)
    {
        sample = analogRead(mPinG);
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
    // double volts = (peakToPeak * 5.0) / 1024;
    double volts = (peakToPeak * 3.3) / 1024;
    return volts;
}

// potentiometer
// -------------
// scaled from 0 - 1
float potentiometerScaledValue()
{
    int potLevel = analogRead(pPinG);
    if(potLevel > pMaxPotLevelG)
    {
        pMaxPotLevelG = potLevel + 1;
    }
    float ratio = (float) potLevel / (float) pMaxPotLevelG;
    return ratio;
}

// display
// ------
// takes mode number and changes display to match
void displayMode(int mode)
{
    int currentPin;
    int currentIndex;
    int matrixElement;
    int state;
    for(int i = 0; i <= dDigitArraySizeG; i++)
    {
        currentPin = dStartPinG + i;
        if(currentPin < dPointPinG)
        {
            currentIndex = i;
        }
        else if(currentPin == dPointPinG)
        {
            continue;
        }
        else
        {
            currentIndex = i - 1;
        }

        matrixElement = dDigitMatrixG[mode][currentIndex];
        state = 0;
        if(matrixElement == 0 )
        {
            state = 1;
        }
        digitalWrite(currentPin, state);
    }
}

void setDisplayPoint(bool display)
{
    int state = 0;
    if(!display)
    {
        state = 1;
    }
    digitalWrite(dPointPinG, state);
}


// ==============
// MODE FUNCTIONS
// ==============

// runs at setup()
void setupInit()
{
    // set hueStepG
    float fHueStepG = 255.0 / (float) lNumG;
    if(fHueStepG < 1.0)
    {
        fHueStepG = 1.0;
    }
    hueStepG = (int) fHueStepG;

    // set previousPotG
    previousPotG = potentiometerScaledValue();
    
    // start mode0
    displayMode(0);
    mode0(previousPotG);
}

void dark()
{
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(0, 0, 0);
    }
    FastLED.show();
}

// ======
// MODES
// ======

// hue changes with pot
void mode0(float p)
{
    // DEBUG
    Serial.println("mode0 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;

    float fHue = 255.0 * p;
    uint8_t hue = (int) fHue;
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
    }
    FastLED.show();
}

// red, brightness changes with pot
// hue 0 == red
void mode1(float p)
{
    // DEBUG
    Serial.println("mode1 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;

    float fValue = 255.0 * p;
    uint8_t value = (int) fValue;
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(0, lDefaultSaturationG, value);
    }
    FastLED.show();
}

// green, brightness changes with pot
// HSV green == 120 / 360 scaled to 85 / 255
void mode2(float p)
{
    // DEBUG
    Serial.println("mode2 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;

    float fValue = 255.0 * p;
    uint8_t value = (int) fValue;
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(85, lDefaultSaturationG, value);
    }
    FastLED.show();
}

// blue, brightness changes with pot
// HSV blue == 240 / 360 scaled to 170 / 255
void mode3(float p)
{
    // DEBUG
    Serial.println("mode3 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;

    float fValue = 255.0 * p;
    uint8_t value = (int) fValue;
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(170, lDefaultSaturationG, value);
    }
    FastLED.show();
}

// rainbow loop
// pot controls rotation speed
void mode4(float p)
{
    // DEBUG
    Serial.println("mode4 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    int currentMillis = millis();
    if(currentMillis > targetMillisG){

        // recompute delay between refreshes
        // max delay hardwired to 1000 (1 second)
        if(p < 0.01)
        {
            p = 0.01;
        }
        float fDelayMillis = 1000.0 * p;
        targetMillisG = currentMillis + (int) fDelayMillis;
        uint8_t hue = previousHueG;
        for(int i = 0; i < lNumG; i++)
        {
            uint8_t tempHue = hue + hueStepG;
            hue = tempHue % 255;
            ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
            if(i == 0)
            {
                 previousHueG = hue;
            }
        }
        FastLED.show();
    }
}

void mode5(float p)
{
    // DEBUG
    Serial.println("mode5 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void mode6(float p)
{
    // DEBUG
    Serial.println("mode6 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void mode7(float p)
{
    // DEBUG
    Serial.println("mode7 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void mode8(float p)
{
    // DEBUG
    Serial.println("mode8 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void mode9(float p)
{
    // DEBUG
    Serial.println("mode9 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void modeA(float p)
{
    // DEBUG
    Serial.println("modeA called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void modeB(float p)
{
    // DEBUG
    Serial.println("modeB called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void modeC(float p)
{
    // DEBUG
    Serial.println("modeC called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void modeD(float p)
{
    // DEBUG
    Serial.println("modeD called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void modeE(float p)
{
    // DEBUG
    Serial.println("modeE called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void modeF(float p)
{
    // DEBUG
    Serial.println("modeF called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
    dark();
}

void modeFail(int m)
{
    Serial.print("invalid mode passed in: ");
    Serial.println(m);
}
