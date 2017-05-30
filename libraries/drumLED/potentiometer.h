// potentiometer.h
// Potentiometer class implementation for Arduino

#ifndef Potentiometer_h
#define Potentiometer_h




// Potentiometer class allows asynchronos calls to a member function that will return true if ther
// has been a button press (only once).
// Handles bounce conditions

class Potentiometer
{
  public:
    Potentiometer(int);
    int Peek();  // instantanious reading from the pin
    float NormalizedPeek(); // normalized to between 0 and 1.
  private:
    int _pin;
    int minValue;
    int maxValue;
};

// the #include statment and code go here...

#endif