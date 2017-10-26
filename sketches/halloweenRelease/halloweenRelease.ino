/*

    halloweenRelease.ino

    This sketch is to run on the arduino
    at dunster road's halloween party 2017

    There have been memory errors
    which have stopped development
    so to push out a release for next week
    there will be changes.

    Only 8 modes
    use #defines instead of globals where applicable
    use uint8_t where applicable


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

// must be < 256
#define LED_NUM 148
//#define LED_NUM 40

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


// =======
// GLOBALS
// =======

// button
// ------
// const uint8_t bPinG = BUTTON_PIN;
bool bPressedG = false;

// this is initial mode
uint8_t bModeG = 0;

// microphone
// ----------
// const uint8_t mPinG = MIC_PIN;

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
// making lNumG uint8_t means there will not be values > 256
const uint8_t lNumG = LED_NUM;
const uint8_t lDefaultValueG = DEFAULT_VALUE;
const uint8_t lDefaultSaturationG = DEFAULT_SATURATION;

// fastLED data structure
CRGB ledsG[lNumG];

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

// config
// ------
const int modeChangeDelayG = MODE_CHANGE_DELAY;
const unsigned long millisUntilOffG = MILLIS_UNTIL_OFF;
const unsigned long maxMillisIterationG = MAX_MILLIS_ITERATION;


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

    FastLED.addLeds<NEOPIXEL, LED_PIN>(ledsG, lNumG);
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
    int potLevel = analogRead(POT_PIN);
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

    if(m == 6)
    {
        for(uint8_t i = 0; i < lNumG; i++)
        {
            savedParamsG[i] = 0;
        }
    }
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
    delay(modeChangeDelayG);
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
    float fRawDelay = maxMillisIterationG * p;
    int rawDelay = round(fRawDelay);

    // flip
    unsigned long nextIteration = maxMillisIterationG - rawDelay;

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

// hue changes with pot
void mode0(float p)
{
    // DEBUG
    Serial.println("mode0 called");

    setDisplayPoint(false);
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
    // DEBUG
    Serial.println("mode1 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    // divide color space among given LEDs
    // FIXME: this code will only work with lNumG < 255
    //        since lNumG is uint8_t this is not a realistic worry ...
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
void mode2(float p)
{
    // DEBUG
    Serial.println("mode2 called");

    setDisplayPoint(false);
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
    uint8_t colorCycleStep = 32;
    uint8_t lastIndex = lNumG - 1;
    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG)
    {
        unsigned long delayMillis = computeNextIteration(p);
        savedTargetMillisG = currentMillis + delayMillis;

        // cycle through colors a bit faster this way
        uint8_t tmp = savedHueG + colorCycleStep;
        uint8_t hue = tmp % 255;
        tmp = savedLedG + 1;
        uint8_t onLed = tmp % lNumG;
        uint8_t offLed = onLed - 1;
        if(onLed == 0)
        {
            offLed = lastIndex;
        }
        savedLedG = onLed;
        ledsG[onLed] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
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
    FastLED.setBrightness(lDefaultValueG);
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
        if(millisSinceLastSample > millisUntilOffG)
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
        if(millisSinceLastSample > millisUntilOffG)
        {
            dark();
        }
    }

    // only update LEDS if there is a change
    if(savedHueG != hue)
    {
        uint8_t j;

        // initialize display array
        uint8_t displayParams[lNumG];
        displayParams[0] = hue;
        uint8_t indexLimit = lNumG - 1;
        for(uint8_t i = 0; i < indexLimit; i++)
        {
            j = i + 1;
            displayParams[j] = savedParamsG[i];
        }

        // use displayParams to populate LEDs, savedParamsG
        for(uint8_t i = 0; i < indexLimit; i++)
        {
            if(displayParams[i] > 0)
            {
                ledsG[i] = CHSV(displayParams[i], lDefaultSaturationG, lDefaultValueG);
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

    setDisplayPoint(true);
    alwaysUpdateG = true;

    // savedParamsG populated by vuOverSavedParamsG() or vuUnderSavedParamsG()

    float rawVolts = readPeakToPeak();
    float cookedVolts = rawVolts * p;

    if(cookedVolts <= mVoltFloorG)
    {
        // check timing
        unsigned long currentMillis = millis();
        unsigned long millisSinceLastSample = currentMillis - savedLastSampleMillisG;
        if(millisSinceLastSample > millisUntilOffG)
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
    Serial.print("invalid mode passed in: ");
    Serial.println(m);
}

