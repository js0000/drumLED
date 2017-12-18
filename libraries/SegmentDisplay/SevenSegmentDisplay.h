// header for seven segment dislay
// use member function Display(number)

#ifndef SegmentDisplay_h
#define SegmentDisplay_h

#include <Prototype.h>


// Seven Segment Display library for Arduino
class SegmentDisplay
{
  public:
    SegmentDisplay(int pin, int pointPin);
    void Display(int digit);
    void SetDecimalPoint(int v);
  private:
    int _pin;
    int _pointPin;
    int _decimalPointOn;
    
};

// the #include statment and code go here...

#endif
