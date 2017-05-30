/*
 * Combined.ino
 */

#include "FastLED.h"

#define NUM_LEDS 30
#define DATA_PIN 6

CRGB leds[NUM_LEDS];

int micPin = 0;
int potPin = 2;

// Sample window width in mS (50 mS = 20Hz)
const int sampleWindow = 50;
unsigned int sample;

double maxVolts = 2.2;
uint8_t previousHue = 0;

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  pinMode(micPin, INPUT);
  pinMode(potPin, INPUT);
}


void loop() {
  double volts = readPeakToPeak();
  int rawHue = voltsToHue(volts);
  uint8_t hue = prepareHue(rawHue);
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, 255);

  }
  FastLED.show();
}

double readPeakToPeak() {
   unsigned long startMillis = millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;

   // collect data for 50 mS
   while(millis() - startMillis < sampleWindow)
   {
      sample = analogRead(micPin);
      if(sample < 1024)  // toss out spurious readings
      {
         if(sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if(sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
   // double rawVolts = (peakToPeak * 5.0) / 1024;  // convert to volts
   double rawVolts = (peakToPeak * 3.3) / 1024;  // convert to volts

   // use pot level as a gain 0-1.0;
   int potLevel = analogRead(potPin);
   double filter = potLevel * (1.0 / 666.0);
   // double filter = (double) potLevel;
   double cookedVolts = rawVolts * filter;

   return cookedVolts;
}


int voltsToHue(double v) {
    double numerator = v * 255.0;
    double ratio = numerator / maxVolts;
    int h = round(ratio);
    return h;
}

uint8_t prepareHue(int rh) {
  int initialHue = rh % 128;

  // yellow = 60
  int offset = 60;

  /*
   * opposite ends of spectrum are 128 steps apart
   *
   * two ways to get from "top" to "bottom"
   * 1. lower numbers
   * 2. higher numbers
   *
   * this code figures which
   */

  if(previousHue > initialHue) {
      offset += 128;
  }
  int cookedHue = initialHue + offset;
  int currentHue = cookedHue % 255;
  previousHue = currentHue % 128;

  return currentHue;
}

