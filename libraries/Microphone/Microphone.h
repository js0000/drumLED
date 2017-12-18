// Microphone.h

#ifndef Microphone_h
#define Microphone_h





class Microphone
{
  public:
  	Microphone(int pin);
    int sample();         // instantanious reading from the pin
    int peakToPeak(int sampleWindow);
  private:
    int _pin;
    int minValue;
    int maxValue;
};

// the #include statment and code go here...

#endif