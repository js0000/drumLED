/*
  SegmentDisplay.cpp - Library for flashing SegmentDisplay code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/

#include "Arduino.h"
#include "SevenSegmentDisplay.h"
int seven_seg_digits2[10][7] = { { 0,0,0,0,0,0,1 },  // = 0
                                 { 0,0,0,0,0,1,0 },  // = 1
                                 { 0,0,0,0,1,0,0 },  // = 2
                                 { 0,0,0,1,0,0,0 },  // = 3
                                 { 0,0,1,0,0,0,0 },  // = 4
                                 { 0,1,0,0,0,0,0 },  // = 5
                                 { 1,0,0,0,0,0,0 },  // = 6
                                 { 1,1,1,1,1,1,1 },  // = 7
                                 { 1,1,1,1,1,1,1 },  // = 8
                                 { 0,0,0,0,0,0,0 }   // = 9
                                 // = A
                                 // = b
                                 // = C
                                 // = d
                                 // = E
                                 // = F

                                 // dot not handled.

 };
int seven_seg_digits[16][7] = { { 0,1,1,1,1,1,1 },  // = 0
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

SegmentDisplay::SegmentDisplay(int pin)
{
  pinMode(13, OUTPUT);   // they have to be in a sequence to make Display function easy to call
  pinMode(12, OUTPUT); 
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);

  pinMode(4, INPUT);  // button should not be here...

  pinMode(pin, OUTPUT);
  _pin = pin;
}

void SegmentDisplay::Display(int digit) {
  int pin = 7;
  for (int segCount = 0; segCount< 7; ++segCount) 
  {
    digitalWrite(pin, seven_seg_digits[digit][segCount]?0:1);
    ++pin;
  }
}
