/*
  SegmentDisplay.cpp - Library for flashing SegmentDisplay code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/

#include "Arduino.h"
#include "SevenSegmentDisplay.h"

#ifdef PROTOTYPE
int seven_seg_digitsTest[16][8] = { 
								// 7 6 5 4 3 2 1 0
//								 { 0,0,1,1,1,1,1,1},  // = 0
								 { 0,0,0,0,0,0,0,1},  // = 0
                                 { 0,0,0,0,0,0,1,0 },  // = 1
                                 { 0,0,0,0,0,1,0,0 },  // = 2
                                 { 0,0,0,0,1,0,0,0  },  // = 3
                                 { 0,1,0,0,0,0,0,0 },  // = 4
                                 { 0,1,0,1,1,0,1,1  },  // = 5
                                 { 0,1,0,1,1,1,1,1 },  // = 6
                                 { 0,0,1,1,0,0,0,1 },  // = 7
                                 { 0,1,1,1,1,1,1,1 },  // = 8
								 { 0,1,1,1,1,0,0,1 },  // = 9
                                 { 0,1,1,1,1,1,0,1},  // = A
              ddd                   { 0,1,0,0,1,1,1,1 },  // = B
                                 { 0,0,0,1,1,1,1,0 },  // = C
                                 { 0,1,1,0,0,1,1,1 },  // = D
                                 { 0,1,0,1,1,1,1,0 },  // = E
                                 { 0,1,0,1,1,1,0,0 }  // = F
                                 // = A
                                 // = b
                                 // = C
                                 // = d
                                 // = E
                                 // = F

                                 // dot not handled.

};

int seven_seg_digits[16][8] = { 
								// 7 6 5 4 3 2 1 0
//								 { 0,0,1,1,1,1,1,1},  // = 0
								 { 0,0,1,1,1,1,1,1},  // = 0
                                 { 0,0,1,0,0,0,0,1 },  // = 1
                                 { 0,1,1,1,0,1,1,0 },  // = 2
                                 { 0,1,1,1,0,0,1,1  },  // = 3
                                 { 0,1,1,0,1,0,0,1 },  // = 4
                                 { 0,1,0,1,1,0,1,1  },  // = 5
                                 { 0,1,0,1,1,1,1,1 },  // = 6
                                 { 0,0,1,1,0,0,0,1 },  // = 7
                                 { 0,1,1,1,1,1,1,1 },  // = 8
								 { 0,1,1,1,1,0,0,1 },  // = 9
                                 { 0,1,1,1,1,1,0,1},  // = A
                                 { 0,1,0,0,1,1,1,1 },  // = B
                                 { 0,0,0,1,1,1,1,0 },  // = C
                                 { 0,1,1,0,0,1,1,1 },  // = D
                                 { 0,1,0,1,1,1,1,0 },  // = E
                                 { 0,1,0,1,1,1,0,0 }  // = F
                                 // = A
                                 // = b
                                 // = C
                                 // = d
                                 // = E
                                 // = F

                                 // dot not handled.

};
#else
int seven_seg_digits[16][8] = {// 7 6 5 4 3 2 1 0
								{ 1,1,1,0,1,1,1,0 },  // = 0
                                { 0,0,1,0,1,0,0,0 },  // = 1
                                { 1,1,0,0,1,1,0,1 },  // = 2
                                { 0,1,1,0,1,1,0,1 },  // = 3
                                { 0,0,1,0,1,0,1,1 },  // = 4
                                { 0,1,1,0,0,1,1,1 },  // = 5
                   jjj             { 1,1,1,0,0,1,1,1 },  // = 6
                            	{ 0,0,1,0,1,1,0,0 },  // = 7
                                { 1,1,1,0,1,1,1,1 },  // = 8
                                { 0,0,1,0,1,1,1,1 },  // = 9
                                { 1,0,1,0,1,1,1,1 },  // = A
                                { 1,1,1,0,0,0,1,1 },  // = 8
                                { 1,1,0,0,0,0,0,1 },  // = C
                                { 1,1,1,0,1,0,0,1 },  // = d
                                { 1,1,0,0,0,1,1,1 },  // = e
                                { 1,0,0,0,0,1,1,1 },  // = f
//                                { 1,1,1,1,1,1,1,1 }  // = e
//                                     { 1,1,1,1,1,1,1 },  // = f

                                 // dot not handled.

};
#endif


int seven_seg_digitsOLD[16][7] = {      { 0,1,1,1,1,1,1 },  // = 0
                                     { 0,1,0,0,0,0,1 },  // = 1
                                     { 1,1,1,0,1,1,0 },  // = 2
                                     { 1,1,1,0,0,1,1 },  // = 3
                                     { 1,1,0,1,0,0,1 },  // = 4
                                     { 1,0,1,1,0,1,1 },  // = 5
                                     { 1,0,1,1,1,1,1 },  // = 6
                                     { 0,1,1,0,0,0,1 },  // = 7
                                     { 1,1,1,1,1,1,1 },  // = 8
                                     { 1,1,1,1,0,0,1 },  // = 9
                                     { 1,1,1,1,1,0,1 },  // = A
                                     { 1,0,0,1,1,1,1 },  // = b
                                     { 0,0,1,1,1,1,0 },  // = C
                                     { 1,1,0,0,1,1,1 },  // = d
                                     { 1,0,1,1,1,1,0 },  // = E
                                     { 1,0,1,1,1,0,0 }   // = F

                                 // dot not handled.

 };

//      bits used in segment display
//               4
//            3     5
//               6
//            2     0
//               1

SegmentDisplay::SegmentDisplay(int startPin, int pointPin)
{

  _pin = startPin;
  _pointPin = pointPin;
  _decimalPointOn=0;
  for (int PIN = _pin; PIN < (_pin + 8); PIN++)
	{
	  	pinMode(PIN, OUTPUT);   // they have to be in a sequence to make Display function easy to call
        digitalWrite(PIN, 1);
	}
	pinMode(_pointPin, OUTPUT);
    digitalWrite(_pointPin, 1);
	
}

void SegmentDisplay::SetDecimalPoint(int v)
{
  Serial.print("SetDecimalPoint ");
  Serial.println(_decimalPointOn);
  _decimalPointOn = v;
}

void SegmentDisplay::Display(int digit) {
  int pin = _pin;
  for (int segCount = 0; segCount< 8; segCount++) 
  {
//     if (pin != _pointPin)
#ifdef PROTOTYPE
        digitalWrite(pin, seven_seg_digits[digit][segCount+1]?0:1);
#else
        digitalWrite(pin, seven_seg_digits[digit][segCount]?0:1);
#endif
        
    pin++;
  }
  Serial.print("Display ");
  Serial.print(_decimalPointOn);

  Serial.println(_pointPin);
  
  if (_decimalPointOn == 0)
  	  digitalWrite(_pointPin, 1);
  else
  	  digitalWrite(_pointPin, 0);
}
