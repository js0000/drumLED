/*

    halloweenRelease.ino

    This sketch is to run on the arduino
    at dunster road's halloween party 2017

    There have been memory errors
    which have stopped development
    so to push out a release for next week
    there will be changes.

    Only 8 modes
    no intermediate values for #defines


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
#define MIC_PIN A3
#define POT_PIN A0
#define DISPLAY_POINT_PIN 9
#define DISPLAY_START_PIN 6
#define LED_PIN 3

// mode4 will not work if this is > 255
#define LED_NUM 148

// both must be between 0-255
// 'value' == brightness
#define DEFAULT_VALUE 64
#define DEFAULT_SATURATION 255

// break between modes: in milliseconds
#define MODE_CHANGE_DELAY 512

// LEDs off if no input within limit
#define MILLIS_UNTIL_OFF 4096

// longest time between iterations of LED changes
#define MAX_MILLIS_ITERATION 2048


// =======
// GLOBALS
// =======

// button
// ------
// const uint8_t bPinG = BUTTON_PIN;
bool bPressedG = false;

// this is initial mode
int bModeG = 0;

// microphone
// ----------
// const uint8_t mPinG = MIC_PIN;

// this is changeable (can increase)
// allows for more coverage of color spectrum
float mMaxVoltsG = 0.6;

// to better fill out the color space
int mMaxRawHueG = 192;
int mMinRawHueG = 32;

// potentiometer
// -------------
// const uint8_t pPinG = POT_PIN;
// pMaxPotLevelG will not be needed with standardized hardware
int pMaxPotLevelG = 672;

// display
// ------
// const uint8_t dPointPinG = DISPLAY_POINT_PIN;
// const uint8_t dStartPinG = DISPLAY_START_PIN;
const uint8_t dDigitArraySizeG = 7;

// led
// ---
// const uint8_t lPinG = LED_PIN;
// const uint8_t lNumG = LED_NUM;
// const uint8_t lDefaultValueG = DEFAULT_VALUE;
// const uint8_t lDefaultSaturationG = DEFAULT_SATURATION;

// fastLED data structure
CRGB ledsG[LED_NUM];

// history
// -------
bool alwaysUpdateG = false;
int savedModeG = 0;
float savedPotG;
unsigned long savedTargetMillisG = 0;
uint8_t savedHueG = 0;
int savedLedG = 0;
int savedLastSampleMillisG;
int savedParamsG[LED_NUM];

// config
// ------
// const uint8_t modeChangeDelayG = MODE_CHANGE_DELAY;
// const unsigned long millisUntilOffG = MILLIS_UNTIL_OFF;
// const unsigned long maxMillisIterationG = MAX_MILLIS_ITERATION;


// =======
// ARDUINO
// =======

void setup()
{
    pinMode(BUTTON_PIN, INPUT);
    pinMode(MIC_PIN, INPUT);
    pinMode(POT_PIN, INPUT);

    // <= i instead of < i
    // due to inclusion of point pin
    // which is not accounted for in dDigitArraySizeG
    for(uint8_t i = 0; i <= dDigitArraySizeG; i++)
    {
        uint8_t displayPin = DISPLAY_START_PIN + i;
        pinMode(displayPin, OUTPUT);
    }

    FastLED.addLeds<NEOPIXEL, LED_PIN>(ledsG, LED_NUM);
    setupInit();

    // FIXME: remove this before prod
    Serial.begin(9600);
}

void loop()
{
    uint8_t mode = buttonGetValue();
    float pot = potentiometerScaled();

    boolean updateMode = false;
    if(mode != savedModeG)
    {
        updateMode = true;
        modeInit();
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
            default:
                modeFail(mode);
        }
    }
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
        uint8_t currentButtonValue = bModeG;
        currentButtonValue += 1;
        bModeG = currentButtonValue % 8;
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
    uint8_t potLevel = analogRead(POT_PIN);
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

// display
// ------
// takes mode number and changes display to match
void displayMode(uint8_t mode)
{
    /*
        digitMatrix bit map

            4
        5       3
            6
        0       2
            1
    */

    uint8_t digitMatrix[16][dDigitArraySizeG] = {
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

    uint8_t currentPin;
    uint8_t currentIndex;
    uint8_t matrixElement;
    uint8_t state;
    for(uint8_t i = 0; i <= dDigitArraySizeG; i++)
    {
        currentPin = DISPLAY_START_PIN + i;
        if(currentPin < DISPLAY_POINT_PIN)
        {
            currentIndex = i;
        }
        else if(currentPin == DISPLAY_POINT_PIN)
        {
            continue;
        }
        else
        {
            currentIndex = i - 1;
        }

        matrixElement = digitMatrix[mode][currentIndex];
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
    uint8_t state = 0;
    if(!display)
    {
        state = 1;
    }
    digitalWrite(DISPLAY_POINT_PIN, state);
}


// ==============
// MODE FUNCTIONS
// ==============

// runs at setup()
void setupInit()
{
    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy(random());

    // set savedPotG
    savedPotG = potentiometerScaled();
}

void dark()
{
    for(uint8_t i = 0; i < LED_NUM; i++)
    {
        ledsG[i] = CHSV(0, 0, 0);
    }
    FastLED.show();
}

void modeInit()
{
    // initialize variables
    savedHueG = 0;
    savedLedG = 0;

    // value > 256 means "blank"
    for(uint8_t i = 0; i < LED_NUM; i++)
    {
        savedParamsG[i] = 512;
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
    static byte heat[LED_NUM];
    bool gReverseDirection = false;

    // Step 1.  Cool down every cell a little
    for( uint8_t i = 0; i < LED_NUM; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((fireCooling * 10) / LED_NUM) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( uint8_t k= LED_NUM - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < fireSparking ) {
      uint8_t y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( uint8_t j = 0; j < LED_NUM; j++) {
      CRGB color = HeatColor( heat[j]);
      uint8_t pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (LED_NUM - 1) - j;
      } else {
        pixelnumber = j;
      }
      ledsG[pixelnumber] = color;
    }
}


// returns zero if below voltFloor
int voltsToHue(float v)
{
    // anything below this is considered silence
    float voltFloor = 0.05;

    uint8_t computedHue = 0;
    if(v > voltFloor)
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
        float voltRange = mMaxVoltsG - voltFloor;
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
    uint8_t hue = round(fHue);
    uint8_t ledNum = LED_NUM;
    for(uint8_t i = 0; i < ledNum; i++)
    {
        ledsG[i] = CHSV(hue, DEFAULT_SATURATION, DEFAULT_VALUE);
    }
    FastLED.show();
}


// rainbow strip
// pot controls rotation speed
void mode1(float p)
{
    // DEBUG
    Serial.println("mode1 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    // divide color space among given LEDs
    // FIXME: this code will only work with LED_NUM < 255
    uint8_t ledNum = LED_NUM;
    float fHueStep = 255.0 / (float) ledNum;
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
        for(uint8_t i = 0; i < ledNum; i++)
        {
            uint8_t tmp = hue + hueStep;
            hue = tmp % 255;
            ledsG[i] = CHSV(hue, DEFAULT_SATURATION, DEFAULT_VALUE);
            if(i == 0)
            {
                 savedHueG = hue;
            }
        }
        FastLED.show();
    }
}

// rainbow all LEDs via time
void mode2(float p)
{
    // DEBUG
    Serial.println("mode2 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    unsigned long currentMillis = millis();
    uint8_t ledNum = LED_NUM;
    if(currentMillis > savedTargetMillisG){
        unsigned long delayMillis = computeNextIteration(p);
        savedTargetMillisG = currentMillis + delayMillis;
        int tmp = savedHueG + 1;
        uint8_t hue = tmp % 255;
        savedHueG = hue;
        for(uint8_t i = 0; i < ledNum; i++)
        {
            ledsG[i] = CHSV(hue, DEFAULT_SATURATION, DEFAULT_VALUE);
        }
        FastLED.show();
    }
}

// rainbow LED
// single LED travels from start to end of strip
// then increments color
void mode3(float p)
{
    // DEBUG
    Serial.println("mode3 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    // how quickly to cover the color space
    uint8_t ledNum = LED_NUM;
    uint8_t colorCycleStep = 32;
    uint8_t lastIndex = ledNum - 1;
    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG)
    {
        unsigned long delayMillis = computeNextIteration(p);
        savedTargetMillisG = currentMillis + delayMillis;

        // cycle through colors a bit faster this way
        uint8_t tmp = savedHueG + colorCycleStep;
        uint8_t hue = tmp % 255;
        tmp = savedLedG + 1;
        uint8_t onLed = tmp % ledNum;
        uint8_t offLed = onLed - 1;
        if(onLed == 0)
        {
            offLed = lastIndex;
        }
        savedLedG = onLed;
        ledsG[onLed] = CHSV(hue, DEFAULT_SATURATION, DEFAULT_VALUE);
        ledsG[offLed] = CHSV(hue, 0, 0);
        FastLED.show();

        if(onLed == lastIndex)
        {
            savedHueG = hue;
        }
    }
}

// fire
void mode4(float p)
{
    // DEBUG
    Serial.println("mode4 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    float fsparking = 150.0 * p;
    uint8_t temp = round(fsparking);
    uint8_t sparking = 50 + temp;

    uint8_t framesPerSecond = 60;
    FastLED.setBrightness(DEFAULT_VALUE);
    Fire2012(sparking); // run simulation frame
    FastLED.show(); // display this frame
    FastLED.delay(1000 / framesPerSecond);
}

// random-ish colors from mic volume
void mode5(float p)
{
    // DEBUG
    Serial.println("mode5 called");

    setDisplayPoint(true);
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
        for(uint8_t i = 0; i < LED_NUM; i++)
        {
            ledsG[i] = CHSV(hue, DEFAULT_SATURATION, DEFAULT_VALUE);
        }
        FastLED.show();

        // reset
        savedHueG = hue;
    }
}


void mode6(float p)
{
    // DEBUG
    Serial.println("mode6 called");

    setDisplayPoint(true);
    alwaysUpdateG = true;

    uint8_t hue = savedHueG;

    float rawVolts = readPeakToPeak();
    float cookedVolts = rawVolts * p;
    uint8_t rawHue = voltsToHue(cookedVolts);

    unsigned long millisUntilOff = MILLIS_UNTIL_OFF;
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
        if(millisSinceLastSample > millisUntilOff)
        {
            dark();
        }
    }

    // only update LEDS if there is a change
    uint8_t ledNum = LED_NUM;
    if(savedHueG != hue)
    {
        uint8_t j;

        // initialize display array
        uint8_t displayParams[ledNum];
        displayParams[0] = hue;
        uint8_t indexLimit = ledNum - 1;
        for(uint8_t i = 0; i < indexLimit; i++)
        {
            j = i + 1;
            displayParams[j] = savedParamsG[i];
        }

        // use displayParams to populate LEDs, savedParamsG
        for(uint8_t i = 0; i < indexLimit; i++)
        {
            if(displayParams[i] < 256)
            {
                ledsG[i] = CHSV(displayParams[i], DEFAULT_SATURATION, DEFAULT_VALUE);
            }
            // larger values mean blanks
            else
            {
                ledsG[i] = CHSV(0, 0, 0);
            }
            savedParamsG[i] = displayParams[i];
        }
        FastLED.show();

        // reset
        savedHueG = hue;
    }
}

// VU meter
void mode7(float p)
{
    // DEBUG
    Serial.println("mode7 called");

    setDisplayPoint(false);
    alwaysUpdateG = false;

}

void modeFail(uint8_t m)
{
    Serial.print("invalid mode passed in: ");
    Serial.println(m);
}

