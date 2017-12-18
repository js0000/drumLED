//button.cpp
#include "Arduino.h"
#include "button.h"



MyButton::MyButton(int inputPin)
{
	_pin=inputPin;
	lastButtonCheck=0;
	pinMode(_pin, INPUT);
}

int MyButton::Peek()
{
	return lastButtonCheck;
}

int MyButton::HasBeenPressed()
{
//  Serial.println("HasBeenPressed\n");
  if (lastButtonCheck==1 )  // handle the fact that a button was previously pressed
  { 
     if (digitalRead(_pin) == 0)
     {  // ready now to see button being pressed
       lastButtonCheck = 0;
//       Serial.println("lo");
       return 1;
      }
     return 0;
  }
  
  if (digitalRead(_pin) == 0)
  {
    return 0;
  } else
  {
    lastButtonCheck=1;
//    Serial.println("hi");
  }
  return 0;
}