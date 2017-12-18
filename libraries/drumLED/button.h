// button.h
// button class implementation for Arduino

#ifndef Button_h
#define Button_h




// button class allows asynchronos calls to a member function that will return true if ther
// has been a button press (only once).
// Handles bounce conditions

class MyButton
{
  public:
    MyButton(int);
    int HasBeenPressed();
    int Peek();
  private:
    int _pin;
    int lastButtonCheck;
};

// the #include statment and code go here...

#endif