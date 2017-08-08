/*
 * CombinedTest.ino
 */

#include "FastLED.h"

#define NUM_LEDS 80
#define LED_PIN 6
#define MIC_PIN 2
#define POT_PIN 0

#define DEBUG_COUNTER 32


// BEGIN CONFIG VARIABLES

// Sample window width in mS (50 mS = 20Hz)
const int sampleWindow = 200;
// anything below this is considered silence
const double voltFloor = 0.05;
// auto off time out
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

// this is changeable (can increase)
// allows for more coverage of color spectrum
double maxVolts = 0.6;
// also chageable for the same reasons
int maxRawHue = 192;
int minRawHue = 32;
unsigned int sample;
uint8_t previousHue = 0;
uint8_t previousSaturation = 0;
// "value" is actually brightness
uint8_t previousValue = 0;
unsigned long lastSampleAboveMinVolts = 0;
int maxPotLevel = 672;

// DEBUGGING

double aVolts[DEBUG_COUNTER];
double aNumerator[DEBUG_COUNTER];
double aRatio[DEBUG_COUNTER];
uint8_t aHue[DEBUG_COUNTER];
int aInvertedHue[DEBUG_COUNTER];
int aScaledHue[DEBUG_COUNTER];
int aPreppedHue[DEBUG_COUNTER];
uint8_t aCurrentHue[DEBUG_COUNTER];
long aMillis[DEBUG_COUNTER];
int counter = 0;


// STOCK ARDUINO CALLS

void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  pinMode(MIC_PIN, INPUT);
  pinMode(POT_PIN, INPUT);
  Serial.begin(9600);
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
    if(sinceLastSample > maxMillisBeforeTurningOff)
    {
      hue = 0;
      saturation = 0;
      value = 0;
    }
  }

  // only update LEDS if there is a change
  if(previousHue != hue || previousSaturation != saturation || previousValue != value)
  {
    for(int i = 0; i < NUM_LEDS; i++)
    {
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
// https://learn.adafruit.com/adafruit-microphone-amplifier-breakout/measuring-sound-levels
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
  if(potLevel > maxPotLevel)
  {
    maxPotLevel = potLevel + 1;
  }
  double numerator = potLevel * 100.0;
  double filter = numerator / maxPotLevel;
  double cookedVolts = v * filter;
  return cookedVolts;
}


// returns zero if below voltFloor
int voltsToHue(double v) {
  int h = 0;
  if(v > voltFloor)
  {
    // recalibrate maxVolts if necessary
    if(v > maxVolts)
    {
      maxVolts = v + 0.01;
    }

    // recalibrate rawHues if necessary
    if(h > maxRawHue)
    {
      maxRawHue = (h + 1);
      if(maxRawHue > 255)
      {
        maxRawHue = 255;
      }
    }
    else if(h < minRawHue)
    {
      minRawHue = (h - 1);
      if(minRawHue < 0)
      {
        minRawHue = 0;
      }
    }
    double hueRange = maxRawHue - minRawHue;
    double numerator = v * hueRange;
    double voltRange = maxVolts - voltFloor;
    double ratio = numerator / voltRange;
    h = round(ratio);

    // DEBUGGING
    if(counter < DEBUG_COUNTER)
    {
      aVolts[counter] = v;
      aNumerator[counter] = numerator;
      aRatio[counter] = ratio;
      aHue[counter] = h;
    }
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
  if(previousHue > rh)
  {
      louder = false;
  }
  // make loudest 0
  int invertedHue = maxRawHue - rh;
  // scale to 128
  int scaledHue = invertedHue / 2;
  // 0-127 louder, 128-255 quieter
  if(!louder)
  {
    // on a circle, not a line
    scaledHue = 256 - scaledHue;
  }
  // offset so loudest matches hueOffset
  int preppedHue = scaledHue + hueOffset;
  // go around the circle, if needed
  uint8_t currentHue = preppedHue % 255;

  // DEBUGGING
  if(counter < DEBUG_COUNTER)
  {
    aInvertedHue[counter] = invertedHue;
    aScaledHue[counter] = scaledHue;
    aPreppedHue[counter] = preppedHue;
    aCurrentHue[counter] = currentHue;
    aMillis[counter] = millis();
    counter += 1;
  }
  else
  {
     serialMonitorDump();
     // reset
     counter = 0;
  }
  // reset
  previousHue = rh;
  return currentHue;
}

void serialMonitorDump() {
  // header
  Serial.println(" ");
  Serial.println("volts,numerator,ratio,hue,invertedHue,scaledHue,preppedHue,currentHue,millisecond");
  // strings and floats do not play well on arduino
  //  http://forum.arduino.cc/index.php/topic,146638.0.html
  for (int i = 0; i < DEBUG_COUNTER; i++)
  {
    Serial.print(aVolts[i]);
    Serial.print(",");
    Serial.print(aNumerator[i]);
    Serial.print(",");
    Serial.print(aRatio[i]);
    Serial.print(",");
    Serial.print(aHue[i]);
    Serial.print(",");
    Serial.print(aInvertedHue[i]);
    Serial.print(",");
    Serial.print(aScaledHue[i]);
    Serial.print(",");
    Serial.print(aPreppedHue[i]);
    Serial.print(",");
    Serial.print(aCurrentHue[i]);
    Serial.print(",");
    Serial.println(aMillis[i]);
  }
  Serial.println(" ");
  int potLevel = analogRead(potPin);
  Serial.print("# (current) potentiometer level: ");
  Serial.println(potLevel);
  Serial.print("# (mutable) maxPotLevel: ");
  Serial.println(maxPotLevel);
  Serial.print("# (mutable) maxVolts: ");
  Serial.println(maxVolts);
  Serial.print("# (mutable) maxRawHue: ");
  Serial.println(maxRawHue);
  Serial.print("# (mutable) minRawHue: ");
  Serial.println(minRawHue);
}
