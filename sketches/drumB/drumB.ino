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
#define MODE_CHANGE_DELAY 256

// LEDs off if no input within limit: in milliseconds
#define TIME_UNTIL_OFF 4000


// =======
// GLOBALS
// =======

// button
// ------
const int bPinG = BUTTON_PIN;
bool bPressedG = false;

// this is initial mode
int bModeG = 0;

// microphone
// ----------
const int mPinG = MIC_PIN;

// this is changeable (can increase)
// allows for more coverage of color spectrum
float mMaxVoltsG = 0.6;

// to better fill out the color space
int mMaxRawHueG = 192;
int mMinRawHueG = 32;

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

// led
// ---
const int lPinG = LED_PIN;
const int lNumG = LED_NUM;
const int lDefaultValueG = DEFAULT_VALUE;
const int lDefaultSaturationG = DEFAULT_SATURATION;

// fastLED data structure
CRGB ledsG[lNumG];

// history
// -------
bool alwaysUpdateG = false;
int savedModeG = 0;
float savedPotG;
unsigned long savedTargetMillisG = 0;
uint8_t savedHueG = 0;
int savedLedG = 0;
int savedLastSampleMillisG;
int savedParamsG[lNumG];

// config
// ------
const int modeChangeDelayG = MODE_CHANGE_DELAY;
const unsigned long timeUntilOffG = TIME_UNTIL_OFF;


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
        int currentButtonValue = bModeG;
        currentButtonValue += 1;
        bModeG = currentButtonValue % 16;
    }
    return bModeG;
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
    // float volts = (peakToPeak * 5.0) / 1024;
    float volts = (peakToPeak * 3.3) / 1024;
    return volts;
}

// potentiometer
// -------------
// scaled from 0 - 1
float potentiometerScaled()
{
    int potLevel = analogRead(pPinG);
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
void displayMode(int mode)
{
    /*
        digitMatrix bit map

            4
        5       3
            6
        0       2
            1
    */

    int digitMatrix[16][dDigitArraySizeG] = {
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
    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy(random());

    // set savedPotG
    savedPotG = potentiometerScaled();
}

void dark()
{
    for(int i = 0; i < lNumG; i++)
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
    for(int i = 0; i < lNumG; i++)
    {
        savedParamsG[i] = 512;
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


void Fire2012(int fs)
{
    // There are two main parameters you can play with to control the look and
    // feel of your fire: COOLING (used in step 1 above), and SPARKING (used
    // in step 3 above).

    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 50, suggested range 20-100
    int fireCooling = 55;

    int fireSparking = fs;

    // Array of temperature readings at each simulation cell
    static byte heat[lNumG];
    bool gReverseDirection = false;

    // Step 1.  Cool down every cell a little
    for( int i = 0; i < lNumG; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((fireCooling * 10) / lNumG) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= lNumG - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < fireSparking ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < lNumG; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (lNumG - 1) - j;
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

    int computedHue = 0;
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
uint8_t scaleHueAll(int rh)
{
    boolean louder = true;
    if(savedHueG > rh)
    {
        louder = false;
    }

    // make loudest 0
    // easier to scale
    int invertedHue = 255 - rh;

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

// simplified rawHue to one of 6 discrete values
// values are in units of 60 (360 / 6)
uint8_t scaleHueSix(int rh)
{
    // make loudest 0
    int invertedHue = 255 - rh;

    // scale to 360 (circular) from 255
    float numerator = invertedHue * 360.0;
    float ratio = numerator / 255.0;

    // scale to multiple of 60
    float rawColorMultiplier = ratio / 60.0;
    int colorMultiplier = round(rawColorMultiplier);
    uint8_t computedHue = colorMultiplier * 60;

/*
    // DEBUG
    // puts out csv of rh,colorMultiplier,computedHue
    Serial.print(rh);
    Serial.print(",");
    Serial.print(colorMultiplier);
    Serial.print(",");
    Serial.println(computedHue);
*/    
    
    return computedHue;
}

int offsetHue(int o, int h, int mH)
{
    int oh = o + h;
    int r = oh % mH;
    return r;
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
    uint8_t value = round(fValue);
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
    uint8_t value = round(fValue);
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
    uint8_t value = round(fValue);
    for(int i = 0; i < lNumG; i++)
    {
        ledsG[i] = CHSV(170, lDefaultSaturationG, value);
    }
    FastLED.show();
}

// rainbow strip
// pot controls rotation speed
void mode4(float p)
{
    // DEBUG
    Serial.println("mode4 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    // divide color space among given LEDs
    // FIXME: this code will only work with LED_NUM < 255
    float fHueStep = 255.0 / (float) lNumG;
    if(fHueStep < 1.0)
    {
        fHueStep = 1.0;
    }
    uint8_t hueStep = round(fHueStep);

    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG){
        // max delay hardwired to 1000 (1 second)
        float fDelayMillis = 1000.0 * p;
        int tmp = round(fDelayMillis);
        savedTargetMillisG = currentMillis + tmp;
        uint8_t hue = savedHueG;
        for(int i = 0; i < lNumG; i++)
        {
            tmp = hue + hueStep;
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

// rainbow time
void mode5(float p)
{
    // DEBUG
    Serial.println("mode5 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG){
        float fDelayMillis = 1000.0 * p;
        int tmp = round(fDelayMillis);
        savedTargetMillisG = currentMillis + tmp;
        tmp = savedHueG + 1;
        uint8_t hue = tmp % 255;
        savedHueG = hue;
        for(int i = 0; i < lNumG; i++)
        {
            ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
        }
        FastLED.show();
    }
}

// rainbow LED
// single LED travels from start to end of strip
// then increments color
void mode6(float p)
{
    // DEBUG
    Serial.println("mode6 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    int colorCycleStep = 16;
    int lastIndex = lNumG - 1;
    unsigned long currentMillis = millis();
    if(currentMillis > savedTargetMillisG)
    {
        float fDelayMillis = 1000.0 * p;
        int tmp = round(fDelayMillis);
        savedTargetMillisG = currentMillis + tmp;

        // cycle through colors a bit faster this way
        tmp = savedHueG + colorCycleStep;
        uint8_t hue = tmp % 255;
        tmp = savedLedG + 1;
        int onLed = tmp % lNumG;
        int offLed = onLed - 1;
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

void mode7(float p)
{
    // DEBUG
    Serial.println("mode7 called");

    setDisplayPoint(false);
    alwaysUpdateG = true;

    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    float fPreSparking = 150.0 * p;
    int temp = round(fPreSparking);

    // flipped so control is more intuitive
    int flippedTemp = 150 - temp;
    int sparking = 50 + flippedTemp;

    int framesPerSecond = 60;
    FastLED.setBrightness(lDefaultValueG);
    Fire2012(sparking); // run simulation frame
    FastLED.show(); // display this frame
    FastLED.delay(1000 / framesPerSecond);
}

void mode8(float p)
{
    // DEBUG
    Serial.println("mode8 called");

    setDisplayPoint(true);
    alwaysUpdateG = true;

    uint8_t hue = savedHueG;

    float rawVolts = readPeakToPeak();
    float cookedVolts = rawVolts * p;
    int rawHue = voltsToHue(cookedVolts);

    if(rawHue != 0)
    {
        int scaledHue = scaleHueAll(rawHue);
        // 43/256 ~= 60/360
        hue = offsetHue(43, scaledHue, 255);
        savedLastSampleMillisG = millis();
    }
    else
    {
        // check timing
        unsigned long currentMillis = millis();
        unsigned long sinceLastSample = currentMillis - savedLastSampleMillisG;
        if(sinceLastSample > timeUntilOffG)
        {
            dark();
        }
    }

    // only update LEDS if there is a change
    if(savedHueG != hue)
    {
        for(int i = 0; i < lNumG; i++)
        {
            ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
        }
        FastLED.show();

        // reset
        savedHueG = hue;
    }
}

void mode9(float p)
{
    // DEBUG
    Serial.println("mode9 called");

    setDisplayPoint(true);
    alwaysUpdateG = true;

    uint8_t hue = savedHueG;

    float rawVolts = readPeakToPeak();
    float cookedVolts = rawVolts * p;
    int rawHue = voltsToHue(cookedVolts);

    if(rawHue != 0)
    {
        int scaledHue = scaleHueSix(rawHue);
        // 213/256 ~= 300/360
        hue = offsetHue(213, scaledHue, 255);
        savedLastSampleMillisG = millis();
    }
    else
    {
        // check timing
        unsigned long currentMillis = millis();
        unsigned long sinceLastSample = currentMillis - savedLastSampleMillisG;
        if(sinceLastSample > timeUntilOffG)
        {
            dark();
        }
    }

    // only update LEDS if there is a change
    if(savedHueG != hue)
    {
        for(int i = 0; i < lNumG; i++)
        {
            ledsG[i] = CHSV(hue, lDefaultSaturationG, lDefaultValueG);
        }
        FastLED.show();

        // reset
        savedHueG = hue;
    }
}

void modeA(float p)
{
    // DEBUG
    Serial.println("modeA called");

    setDisplayPoint(true);
    alwaysUpdateG = true;

    uint8_t hue = savedHueG;

    float rawVolts = readPeakToPeak();
    float cookedVolts = rawVolts * p;
    int rawHue = voltsToHue(cookedVolts);

    if(rawHue != 0)
    {
        int scaledHue = scaleHueAll(rawHue);
        // 43/256 ~= 60/360
        hue = offsetHue(43, scaledHue, 255);
        savedLastSampleMillisG = millis();
    }
    else
    {
        // check timing
        unsigned long currentMillis = millis();
        unsigned long sinceLastSample = currentMillis - savedLastSampleMillisG;
        if(sinceLastSample > timeUntilOffG)
        {
            dark();
        }
    }

    // only update LEDS if there is a change
    if(savedHueG != hue)
    {
        int j;

        // initialize display array
        int displayParams[lNumG];
        displayParams[0] = hue;
        int indexLimit = lNumG - 1;
        for(int i = 0; i < indexLimit; i++)
        {
            j = i + 1;
            displayParams[j] = savedParamsG[i];
        }

        // use displayParams to populate LEDs, savedParamsG
        for(int i = 0; i < indexLimit; i++)
        {
            if(displayParams[i] < 256)
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

void modeB(float p)
{
    // DEBUG
    Serial.println("modeB called");

    setDisplayPoint(false);
    alwaysUpdateG = false;

    // VU meter
}

void modeC(float p)
{
    // DEBUG
    Serial.println("modeC called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
}

void modeD(float p)
{
    // DEBUG
    Serial.println("modeD called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
}

void modeE(float p)
{
    // DEBUG
    Serial.println("modeE called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
}

void modeF(float p)
{
    // DEBUG
    Serial.println("modeF called");

    setDisplayPoint(false);
    alwaysUpdateG = false;
}

void modeFail(int m)
{
    Serial.print("invalid mode passed in: ");
    Serial.println(m);
}

