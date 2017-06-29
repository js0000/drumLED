/*
 * CombinedTest.ino
 */

#include "FastLED.h"

#define NUM_LEDS 30
#define LED_PIN 6
#define MIC_PIN 0
#define POT_PIN 2


// BEGIN CONFIG VARIABLES

// Sample window width in mS (50 mS = 20Hz)
const int sampleWindow = 50;
const double maxVolts = 2.2;
const double minVolts = 0.2;
const unsigned long maxMillisBeforeTurningOff = 4000;
// yellow = 60
// this is for the max volume hue
const int hueOffset = 60;

// END CONFIG VARIABLES


// HARDWARE

const int micPin = MIC_PIN;
const int potPin = POT_PIN;
CRGB leds[NUM_LEDS];


// STATE VARIABLES

unsigned int sample;
uint8_t previousHue = 0;
uint8_t previousSaturation = 0;
// "value" is actually brightness
uint8_t previousValue = 0;
unsigned long lastSampleAboveMinVolts = 0;


// STOCK ARDUINO CALLS

void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  pinMode(MIC_PIN, INPUT);
  pinMode(POT_PIN, INPUT);
}

void loop() {
  combinedTest();
}


// main() equivalent
void combinedTest() {
  uint8_t value = previousValue;
  uint8_t saturation = previousSaturation;
  uint8_t hue = previousHue;

  double rawVolts = readPeakToPeak();
  double cookedVolts = applyPotFilter(rawVolts);
  int rawHue = voltsToHue(cookedVolts);

  if(rawHue != 0)
  {
    hue = prepareHue(rawHue);
    // these are the default values for showing a color
    saturation = 255;
    value = 128;
    lastSampleAboveMinVolts = millis();
  }
  else
  {
    // check timing
    unsigned long now = millis();
    unsigned long sinceLastSample = now - lastSampleAboveMinVolts;
    if(sinceLastSample > maxMillisBeforeTurningOff) {
      hue = 0;
      saturation = 0;
      value = 0;
    }
  }

  // only update LEDS if there is a change
  if(previousHue != hue || previousSaturation != saturation || previousValue != value)
  {
    for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue, saturation, value);
    }
    FastLED.show();

    // reset
    previousHue = hue;
    previousSaturation = saturation;
    previousValue = value;
  }
}


// pretty much stolen from adafruit
double readPeakToPeak() {
   unsigned long startMillis = millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;

   // collect data for sampleWindow
   while(millis() - startMillis < sampleWindow)
   {
      sample = analogRead(micPin);
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


// use pot level as a gain 0-1.0;
double applyPotFilter(double v) {
   int potLevel = analogRead(potPin);
   // double filter = (double) potLevel;
   double filter = potLevel * (1.0 / 666.0);
   double cookedVolts = v * filter;
   return cookedVolts;
}


// returns zero if below minVolts
int voltsToHue(double v) {
    int h = 0;
    if( v > minVolts) {
        double numerator = v * 255.0;
        double ratio = numerator / maxVolts;
        h = round(ratio);
    }
    return h;
}

// prepares raw hue to be put on HSV color scheme
// compares against previous value
//   to determine which "side" of hue cirle
//   to place value upon
// offsets so yellow is max
uint8_t prepareHue(int rh) {
  boolean louder = true;
  if(previousHue > rh) {
      louder = false;
  }
  // make loudest 0
  int invertedHue = 256 - rh;
  // scale to 128
  int scaledHue = invertedHue / 2;
  // 0-127 louder, 128-255 quieter
  if(!louder) {
    // on a circle, not a line
    scaledHue = 256 - scaledHue;
  }
  // offset so loudest matches hueOffset
  int preppedHue = scaledHue + hueOffset;
  // go around the circle, if needed
  uint8_t currentHue = preppedHue % 255;
  // reset
  previousHue = rh;
  return currentHue;
}
