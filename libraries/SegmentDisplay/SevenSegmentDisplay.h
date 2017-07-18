// header for seven segment dislay
// use member function Display(number)

#ifndef SegmentDisplay_h
#define SegmentDisplay_h




// Seven Segment Display library for Arduino
class SegmentDisplay
{
  public:
    SegmentDisplay(int pin);
    void Display(int digit);
  private:
    int _pin;
};

// the #include statment and code go here...

#endif
