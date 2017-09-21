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


// ======
// DEFINES
// ======

#include "FastLED.h"

#define BUTTON_PIN 4
#define MIC_PIN 2
#define POT_PIN 1
#define DISPLAY_CONTROL_PIN 0
#define LED_NUM 30
#define DISPLAY_POINT_PIN 9
#define DISPLAY_START_PIN 6
#define LED_PIN 3


// ======
// GLOBALS
// ======

// button
// ------
const int bPinG = BUTTON_PIN;
bool bPressedG = false;
int bValueG = 0;

// microphone
// ------
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
int pPinG = POT_PIN;
// pMaxPotLevelG will not be needed with standardized hardware
int pMaxPotLevelG = 672;

// display
// ------
const int dControlPinG = DISPLAY_CONTROL_PIN;
const int dDigitArraySizeG = 7;
const int dPointPinG = DISPLAY_POINT_PIN;
const int dStartPinG = DISPLAY_START_PIN;
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

// fastLED data structure
CRGB ledsG[LED_NUM];

// mode history
// ----
int previousModeG = 0;


// ======
// ARDUINO
// ======

void setup()
{
    pinMode(bPinG, INPUT);
    pinMode(mPinG, INPUT);
    pinMode(pPinG, INPUT);
    pinMode(dControlPinG, OUTPUT);

    // <= because point pin is embedded
    for(int i = 0; i <= dDigitArraySizeG; i++)
    {
        int displayPin = dStartPinG + i;
        pinMode(displayPin, OUTPUT);
    }

    FastLED.addLeds<NEOPIXEL, lPinG>(ledsG, lNumG);

    // initialize
    setDisplayPoint(0);
    displayMode(0);
    mode0();

    // FIXME: remove this before prod
    Serial.begin(9600);
}

void loop()
{
    int mode = buttonGetValue();
    if(mode != previousModeG)
    {
        previousModeG = mode;
        displayMode(mode);
        switch(mode)
        {
            case 0:
                mode0();
                break;
            case 1:
                mode1();
                break;
            case 2:
                mode2();
                break;
            case 3:
                mode3();
                break;
            case 4:
                mode4();
                break;
            case 5:
                mode5();
                break;
            case 6:
                mode6();
                break;
            case 7:
                mode7();
                break;
            case 8:
                mode8();
                break;
            case 9:
                mode9();
                break;
            case 10:
                modeA();
                break;
            case 11:
                modeB();
                break;
            case 12:
                modeC();
                break;
            case 13:
                modeD();
                break;
            case 14:
                modeE();
                break;
            case 15:
                modeF();
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
    delay(1000);
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
// ------
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
// ------
// use pot level as a gain 0-1.0;
double applyPotFilter(double v)
{
    int potLevel = analogRead(pPinG);
    if(potLevel > pMaxPotLevelG)
    {
        pMaxPotLevelG = potLevel + 1;
    }
    double numerator = potLevel * 100.0;
    double filter = numerator / pMaxPotLevelG;
    double cookedValue = v * filter;
    return cookedValue;
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

// 1 on, 0 off
void setDisplayPoint(int display)
{
    int state = 1;
    if(display == 1)
    {
        state = 0;
    }
    digitalWrite(dPointPinG, state);
}


// ======
// MODES
// ======

void mode0()
{
    Serial.println("mode0 called");
}

void mode1()
{
    Serial.println("mode1 called");
}

void mode2()
{
    Serial.println("mode2 called");
}

void mode3()
{
    Serial.println("mode3 called");
}

void mode4()
{
    Serial.println("mode4 called");
}

void mode5()
{
    Serial.println("mode5 called");
}

void mode6()
{
    Serial.println("mode6 called");
}

void mode7()
{
    Serial.println("mode7 called");
}

void mode8()
{
    Serial.println("mode8 called");
}

void mode9()
{
    Serial.println("mode9 called");
}

void modeA()
{
    Serial.println("modeA called");
}

void modeB()
{
    Serial.println("modeB called");
}

void modeC()
{
    Serial.println("modeC called");
}

void modeD()
{
    Serial.println("modeD called");
}

void modeE()
{
    Serial.println("modeE called");
}

void modeF()
{
    Serial.println("modeF called");
}

void modeFail(int m)
{
    Serial.print("invalid mode passed in: ");
    Serial.println(m);
}
