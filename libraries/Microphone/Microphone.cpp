#include "Arduino.h"
#include "Microphone.h"


Microphone::Microphone(int inputPin)
{
  _pin=inputPin;
  minValue=0;
  maxValue=100;

  pinMode(_pin, INPUT);  
}

int Microphone::sample()
{  // returns an instant readout of microphone input
  return analogRead(_pin);
}

int Microphone::peakToPeak(int sampleWindow)
{  // examines microphone input for sampleWindow milliseconds, determines the pk-pk level
   //   and retuns it.

   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level
   unsigned int potLevel;
   
   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;

   unsigned int sample;

   // collect data for 50 mS and determine the minimum and maximum values
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(_pin);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
//   Serial.println("peakToPeak: ");
//   Serial.println(peakToPeak);
   return peakToPeak;


}

