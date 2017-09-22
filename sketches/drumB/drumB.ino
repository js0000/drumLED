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
#define DISPLAY_CONTROL_PIN 0
#define DISPLAY_POINT_PIN 9
#define DISPLAY_START_PIN 6
#define LED_PIN 3
#define LED_NUM 30


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
const int dControlPinG = DISPLAY_CONTROL_PIN;
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
const float fColorStepG = 255.0 / (float) lNumG;
const uint8_t colorStepG = (int) fColorStepG;

// fastLED data structure
CRGB ledsG[lNumG];

// history
// -------
bool alwaysUpdateG = false;
int previousModeG = 0;
float previousPotG;
int previousMillisG = 0;
uint8_t previousHueG = 0;

// =======
// ARDUINO
// =======

void setup()
{
    pinMode(bPinG, INPUT);
    pinMode(mPinG, INPUT);
    pinMode(pPinG, INPUT);
    pinMode(dControlPinG, OUTPUT);

    // <= i instead of < i
    // due to inclusion of point pin
    // which is not accounted for in dDigitArraySizeG
    for(int i = 0; i <= dDigitArraySizeG; i++)
    {
        int displayPin = dStartPinG + i;
        pinMode(displayPin, OUTPUT);
    }

    FastLED.addLeds<NEOPIXEL, lPinG>(ledsG, lNumG);

    // initialize
    setDisplayPoint(0);
    displayMode(0);
    previousPotG = potentiometerScaledValue();
    mode0(previousPotG);

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

        // DEBUG
        Serial.print("pot: ");
        Serial.println(pot);
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

// hue changes with pot
void mode0(float p)
{
    alwaysUpdateG = false;

    // brightness at half
    uint8_t value = 127;
    uint8_t saturation = 255;
    float fHue = 255.0 * p;
    uint8_t hue = (int) fHue;
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(hue, saturation, value);
    }
    FastLED.show();
}

// red, brightness changes with pot
void mode1(float p)
{
    alwaysUpdateG = false;

    float fValue = 255.0 * p;
    uint8_t value = (int) fValue;
    uint8_t saturation = 255;
    uint8_t hue = 0;
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(hue, saturation, value);
    }
    FastLED.show();
}

// green, brightness changes with pot
void mode2(float p)
{
    alwaysUpdateG = false;

    float fValue = 255.0 * p;
    uint8_t value = (int) fValue;
    uint8_t saturation = 255;
    // 120 / 360 scaled to 85 / 255
    uint8_t hue = 85;
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(hue, saturation, value);
    }
    FastLED.show();
}

// blue, brightness changes with pot
void mode3(float p)
{
    alwaysUpdateG = false;

    float fValue = 255.0 * p;
    uint8_t value = (int) fValue;
    uint8_t saturation = 255;
    // 240 / 360 scaled to 170 / 255
    uint8_t hue = 170;
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(hue, saturation, value);
    }
    FastLED.show();
}

// rainbow, pot controls rotation
void mode4(float p)
{
    alwaysUpdateG = true;
    int currentMillis = millis();
    float fDelayMillis = 1000.0 * p;
    int delayMillis = (int) fDelayMillis;
    int offsetMillis = currentMillis - previousMillisG;
    if(offsetMillis > delayMillis)
    {
        uint8_t value = 127;
        uint8_t saturation = 255;
        uint8_t hue = previousHueG;
        for(int i = 0; i < lNumG; i++)
        {
            uint8_t tempHue = hue + colorStepG;
            hue = tempHue % 255;
            ledsG[i] = CHSV(hue, saturation, value);
            if(i == 0)
            {
                 previousHueG = hue;
            }
        }
        FastLED.show();
    }
    previousMillisG = currentMillis;
}

void mode5(float p)
{
    Serial.println("mode5 called");
}

void mode6(float p)
{
    Serial.println("mode6 called");
}

void mode7(float p)
{
    Serial.println("mode7 called");
}

void mode8(float p)
{
    Serial.println("mode8 called");
}

void mode9(float p)
{
    Serial.println("mode9 called");
}

void modeA(float p)
{
    Serial.println("modeA called");
}

void modeB(float p)
{
    Serial.println("modeB called");
}

void modeC(float p)
{
    Serial.println("modeC called");
}

void modeD(float p)
{
    Serial.println("modeD called");
}

void modeE(float p)
{
    Serial.println("modeE called");
}

void modeF(float p)
{
    Serial.println("modeF called");
}

void modeFail(int m)
{
    Serial.print("invalid mode passed in: ");
    Serial.println(m);
}
