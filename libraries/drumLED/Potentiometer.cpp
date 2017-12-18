// Potentiometer.cpp
//button.cpp

#include "Arduino.h"
#include "Potentiometer.h"



Potentiometer::Potentiometer(int inputPin)
{
	_pin=inputPin;
  minValue=0;
  maxValue=781;

	pinMode(_pin, INPUT);
}

int Potentiometer::Peek()
{
	int value;

	value = analogRead(_pin);
	return value;
}

float Potentiometer::NormalizedPeek()
{
   int lastPeek = Peek();
   if (minValue > lastPeek)
    minValue = lastPeek;
   if (maxValue < lastPeek)
    maxValue = lastPeek;

  return (lastPeek-minValue)/(maxValue-minValue);
}
