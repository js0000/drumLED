/*

    Drumβ (beta)
    skipped unicode char in file name

    This sketch is to gather all the code
    for drumLED into one giant file
    to make sure it works as expected

    Then it should be rewritten as library

    inputs
    - button
    - potentiometer
    - microphone

    outputs
    - LED strip
    - display

    (FIXME: standardize pin assignments)

*/


#define BUTTON_PIN 3
#define MIC_PIN 2
#define POT_PIN 1

#define LED_PIN 6
#define LED_NUM 30

// must be contiguous (would rather count from 0, but ...)
#define D_1_PIN 7
#define D_2_PIN 8
#define D_3_PIN 9
#define D_4_PIN 10
#define D_5_PIN 11
#define D_6_PIN 12
#define D_7_PIN 13
#define D_CONTROL_PIN 0
#define D_INPUT_PIN 4

#include "FastLED.h"


// GLOBALS

// button
const int bPinG = BUTTON_PIN;
bool bPressedG = true;
int bValueG = 0x0;

// display
const int dStartPinG = D_1_PIN;
const int dSegmentCountG= 7;
const int dMatrixG[16][dSegmentCount] = {
    { 0,1,1,1,1,1,1 },  // = 0
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
};

/*
dMatrixG bits map

    4
3       5
    6
2       0
    1

*/


// ARDUINO

void setup()
{
    pinMode(BUTTON_PIN, INPUT);
    pinMode(POT_PIN, INPUT);
    pinMode(MIC_PIN, INPUT);

    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_NUM);
    pinMode(D_1_PIN, OUTPUT);
    pinMode(D_2_PIN, OUTPUT);
    pinMode(D_3_PIN, OUTPUT);
    pinMode(D_4_PIN, OUTPUT);
    pinMode(D_5_PIN, OUTPUT);
    pinMode(D_6_PIN, OUTPUT);
    pinMode(D_7_PIN, OUTPUT);
    pinMode(D_CONTROL_PIN, OUTPUT);
    pinMode(D_INPUT_PIN, INPUT);

    Serial.begin(9600);
}

void loop()
{
    int mode = buttonGetValue();
    displayMode(mode);
    switch(mode)
    {
        case 0x0:
            mode0();
            break;
        case 0x1:
            mode1();
            break;
        case 0x2:
            mode2();
            break;
        case 0x3:
            mode3();
            break;
        case 0x4:
            mode4();
            break;
        case 0x5:
            mode5();
            break;
        case 0x6:
            mode6();
            break;
        case 0x7:
            mode7();
            break;
        case 0x8:
            mode8();
            break;
        case 0x9:
            mode9();
            break;
        case 0xA:
            modeA();
            break;
        case 0xB:
            modeB();
            break;
        case 0xC:
            modeC();
            break;
        case 0xD:
            modeD();
            break;
        case 0xE:
            modeE();
            break;
        case 0xF:
            modeF();
            break;
        default:
            modeFail(mode);
    }

    // FIXME: remove, this is only for testing meta-code
    delay(1000);
}


// META-MODE

// buttons
int buttonGetValue()
{
    bool pressed = buttonWasPressed();
    int currentButtonValue = bValueG
    if(pressed)
    {
        currentButtonValue += 0x1;
    }
    bValueG = currentButtonValue % 0xF;
    return bValueG;
}

bool buttonWasPressed()
{
    bool pressedState false;
    int currentState = digitalRead(bPinG);
    if(bPressedG)
    {
        if(currentState == 0)
        {
            bPressedG = false;
            pressedState = true;
        }
    }
    else
    {
        if(currentState == 1)
        {
            bPressedG = true;
        }
    }
    return pressedState;
}


// display
void displayMode(m)
{
    Serial.print("mode: ");
    Serial.println(m);

    int i = dStartPinG;
    for(int j = 0; j < dSegmentCountG; j++)
    {
        int state = dMatrixG[m][j];
        digitalWrite(i, state);
        i += 1;
    }
}


// MODES

void mode0()
{
    Serial.println("mode0 called");
}

void mode1()
{
    Serial.println("mode1 called");
}

void mode2()
{
    Serial.println("mode2 called");
}

void mode3()
{
    Serial.println("mode3 called");
}

void mode4()
{
    Serial.println("mode4 called");
}

void mode5()
{
    Serial.println("mode5 called");
}

void mode6()
{
    Serial.println("mode6 called");
}

void mode7()
{
    Serial.println("mode7 called");
}

void mode8()
{
    Serial.println("mode8 called");
}

void mode9()
{
    Serial.println("mode9 called");
}

void modeA()
{
    Serial.println("modeA called");
}

void modeB()
{
    Serial.println("modeB called");
}

void modeC()
{
    Serial.println("modeC called");
}

void modeD()
{
    Serial.println("modeD called");
}

void modeE()
{
    Serial.println("modeE called");
}

void modeF()
{
    Serial.println("modeF called");
}

void modeFail(m)
{
    Serial.print("invalid mode passed in: ");
    Serial.println(m);
}
