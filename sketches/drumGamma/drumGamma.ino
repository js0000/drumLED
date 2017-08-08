
#include <Prototype.h>

#include <Microphone.h>
#include <Potentiometer.h>
#include <SevenSegmentDisplay.h>
#include <button.h>

#include "FastLED.h"

// Sound_to_light_with_control

#define ONE_POT_BOARD 0
/*
Sound to light example

segment display not implemented just yet.

This is an example program to test the controls of the sound to light project.
#define PROTOBOARD 1
two potentiometers on 
pin A1 used for modulating the current mode
pin A2 used for setting the brighness 
Sound sensor is on A0

*/


#include <Adafruit_NeoPixel.h>

#ifdef PROTOTYPE
#define LED_CONTROL_PIN 6
#else
#define LED_CONTROL_PIN 3
#endif

#define LED_STRING_LENGTH 100
#define NUM_LEDS LED_STRING_LENGTH
CRGB leds[LED_STRING_LENGTH];


#define COLOR_WHITE	0xFFFFFF

#define COLOR_RED	0xFF0000
#define COLOR_GREEN     0x00FF00
#define COLOR_BLUE	0x0000FF
#define COLOR_YELLOW	0xFFFF00
#define COLOR_CYAN 0x00FFFF
#define COLOR_MAGENTA 0xFF00FF
#define COLOR_SILVER	0xC0C0C0
#define COLOR_BROWN	0x8B4513
#define COLOR_ORANGE    0xFF8800
#define COLOR_SALMON    0xFFA07A

long primaryColors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE};
long secondaryColors[] = {COLOR_CYAN, COLOR_MAGENTA, COLOR_YELLOW};
long interestingColors[] = {COLOR_BROWN, COLOR_ORANGE, COLOR_SALMON};

//define the input pins that are attached to each control
#ifdef PROTOTYPE 
#define PROTOBOARD_PIN 2
#define SEGMENT_DISPLAY_START_PIN 7
#define SEGMENT_DISPLAY_POINT_PIN 3
#else
#define SEGMENT_DISPLAY_START_PIN 6
#define SEGMENT_DISPLAY_POINT_PIN 9
#endif

#define MODE_BUTTON_PIN 4
#define BRIGHTNESS_POT_PIN A0
  // mode control is the potentiometer control for each mode
#define MODE_CONTROL_POT_PIN A1
#define MICROPHONE_PIN A2

#define POT_1_PIN A0
#define POT_2_PIN A1
#define POT_3_PIN A3

// fire defines
#define FRAMES_PER_SECOND 60

// Each contol or display has it's own class
SegmentDisplay segmentdisplay(SEGMENT_DISPLAY_START_PIN, SEGMENT_DISPLAY_POINT_PIN);
MyButton modeButton(MODE_BUTTON_PIN);
Potentiometer ControlPot(MODE_CONTROL_POT_PIN);
Potentiometer BrightnessContol(BRIGHTNESS_POT_PIN);  // potentiometr a2 that controls the brightness of the strip
Microphone audioMic(MICROPHONE_PIN);
Potentiometer ThirdPot(POT_3_PIN);

// Using Adafruit library....
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, LED_CONTROL_PIN, NEO_GRB + NEO_KHZ800);


int stripSetOneColor(uint32_t color);

// SETUPS

// the setup routine runs once when you press reset:
void setup() {   
  
  Serial.begin(9600);      // open the serial port at 9600 bps:    
  
  segmentdisplay.Display(0); 
  segmentdisplay.SetDecimalPoint(0);

  //FastLED strip
  FastLED.addLeds<NEOPIXEL, LED_CONTROL_PIN>(leds, LED_STRING_LENGTH);
  
}

void loop() {
int Value;
        int R,G,B;
        int color=COLOR_WHITE;
      // Startup mode value (count)
  static int mode=0;
  if ( modeButton.HasBeenPressed() == 1) 
  {  mode++;
     if (mode > 15) mode = 0;
  }
  segmentdisplay.Display(mode); 


  int colorIndex = 3 * ControlPot.NormalizedPeek();
//  Serial.println(colorIndex);
  int newColor;
  static uint8_t hue=0; 

int val = analogRead(MODE_CONTROL_POT_PIN);
int val2 = analogRead(BRIGHTNESS_POT_PIN); // this is pot ONE
int val3 = analogRead(POT_3_PIN);

//declare variable used switch statement
int brightness = 0;
int sat1;
int hue1;
int red;
int green;
int blue;

if (mode !=7)
segmentdisplay.SetDecimalPoint(0);
else
segmentdisplay.SetDecimalPoint(1);

switch (mode)
  {
     case 0:
        brightness = min(255, map(val2, 0, 666, 0, 255));
        FastLED.setBrightness(brightness);
        stripSetOneColor(COLOR_WHITE);
     break;    
     case 1:   
//        Serial.println(colorIndex);
        brightness = min(255, map(val2, 0, 666, 0, 255));
        FastLED.setBrightness(brightness);
        stripSetOneColor(primaryColors[colorIndex]);
     break;
     case 2:
        brightness = min(255, map(val2, 0, 666, 0, 255));
        FastLED.setBrightness(brightness);
        stripSetOneColor(secondaryColors[colorIndex]);
     break;
     case 3:
        brightness = min(255, map(val2, 0, 666, 0, 255));
        FastLED.setBrightness(brightness);
        stripSetOneColor(interestingColors[colorIndex]);
     break;
     case 4:
        sat1 = min(255,map(val, 0, 666, 0, 255));
        hue1 = min(255, map(val2, 0, 666, 0, 255));
        brightness = min(255, map(val3, 0, 666, 0, 255));
        
        FastLED.clear();
        for(int led = 0; led < LED_STRING_LENGTH; led++) { 
            leds[led].setHSV( hue1, sat1, brightness);
        }
//        Serial.print("hue:");        
//        Serial.println(hue);
        FastLED.setBrightness(255);
        FastLED.show();
        break;
     case 5:
        red = min(255,map(val, 0, 666, 0, 255));
        green = min(255, map(val2, 0, 666, 0, 255));
        blue = min(255, map(val3, 0, 666, 0, 255));

        // First, clear the existing led s
        FastLED.clear();
        for(int led = 0; led < LED_STRING_LENGTH; led++) { 
            leds[led].setRGB( red, green, blue);
        }
        FastLED.show();
        break;
     case 6:
        red = min(255,map(val, 0, 666, 0, 255));
        green = min(255, map(val2, 0, 666, 0, 255));
        blue = min(255, map(val3, 0, 666, 0, 255));

        // First, clear the existing led s
        FastLED.clear();
        for(int led = 0; led < LED_STRING_LENGTH; led++) { 
            leds[led].setRGB( red, green, blue);
        }
        brightness = audioMic.peakToPeak(50);
        FastLED.setBrightness(brightness);
        FastLED.show();
        
     break;
     case 7:
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy( random());
 
  Fire2012(); // run simulation frame
  FastLED.show(); // display this frame
 
#if defined(FASTLED_VERSION) && (FASTLED_VERSION >= 2001000)
  FastLED.delay(1000 / FRAMES_PER_SECOND);
#else  
  delay(1000 / FRAMES_PER_SECOND);
#endif  ﻿
     break;
     case 8:
//      Serial.print("audioMic.peakToPeak(): ");
//      Serial.println(audioMic.peakToPeak(50));

      rainbowStrip(audioMic.peakToPeak(50));
      break;
     case 9:
        rainbowStrip(ControlPot.Peek());    
//        Serial.println(" finished rainbowCycle ");
     break;
//     default:
//        Serial.println(" passing thru ");
  }

  
  
//  Serial.print("audioMic.sample(): ");
//  Serial.println(audioMic.sample());
  
//  Serial.println("end loop ");
  
}

void setTheBrightness(int Pot)
{
  static uint8_t brightness;
  static int maximum_pot_ =781;  // arbitrary maximum  for this potentiometer

  if (maximum_pot_ < Pot)
    maximum_pot_ = Pot;
    
  unsigned long tempBrightness = (Pot *255.0)/maximum_pot_;
  tempBrightness = max(0, min(255, tempBrightness));
  if (brightness != tempBrightness)
  {
    brightness = tempBrightness;
//    Serial.print(" setting the brightness:"); Serial.println(brightness);
    strip.setBrightness(brightness);
  }
}

void    checkBrightnessPot()
{   
  static int savedPotB = 0;
   
   int potB =  BrightnessContol.Peek();    

  if (savedPotB != potB)
  {
    savedPotB = potB;
    setTheBrightness(savedPotB);
    
  }
}


// Input a  0 to 255 to get a color .
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void mySetPixelColor( uint16_t n, uint8_t r, uint8_t g, uint8_t b) 
{
 strip.setPixelColor(n, r, g, b);
}

void mySetPixelColor(uint16_t n, uint32_t c)
{
 strip.setPixelColor(n, c);
  
}
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowStrip(int offset) {
  uint16_t i, j;
  double scale;
  int color;
  unsigned int potentiometer;
  
   checkBrightnessPot();

//  we use Pot A to swipe thru the rainbow and have it move along the strand
   scale=offset/677.0;  
   color = 255*scale;
   for(i=0; i< strip.numPixels() ; i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + color) & 255));
   }
   strip.show();
}

int stripSetOneColor(uint32_t color)
{
  uint16_t i, j;  
     checkBrightnessPot();
        FastLED.clear();
        for(int led = 0; led < LED_STRING_LENGTH; led++) { 
            leds[led]= color;
        }
        FastLED.show();
 
 }

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation,
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking.
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#define COOLING  55
 
// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120
 
 
void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];
 
  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
 
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 3; k > 0; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
   
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }
 
    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
        leds[j] = HeatColor( heat[j]);
    }
}
 
 
 
// CRGB HeatColor( uint8_t temperature)
// [to be included in the forthcoming FastLED v2.1]
//
// Approximates a 'black body radiation' spectrum for
// a given 'heat' level.  This is useful for animations of 'fire'.
// Heat is specified as an arbitrary scale from 0 (cool) to 255 (hot).
// This is NOT a chromatically correct 'black body radiation'
// spectrum, but it's surprisingly close, and it's extremely fast and small.
//
// On AVR/Arduino, this typically takes around 70 bytes of program memory,
// versus 768 bytes for a full 256-entry RGB lookup table.
 
CRGB HeatColor( uint8_t temperature)
{
  CRGB heatcolor;
 
  // Scale 'heat' down from 0-255 to 0-191,
  // which can then be easily divided into three
  // equal 'thirds' of 64 units each.
  uint8_t t192 = scale8_video( temperature, 192);
 
  // calculate a  that ramps up from
  // zero to 255 in each 'third' of the scale.
  uint8_t heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // now figure out which third of the spectrum we're in:
  if( t192 & 0x80) {
    // we're in the hottest third
    heatcolor.r = 255; // full red
    heatcolor.g = 255; // full green
    heatcolor.b = heatramp; // ramp up blue
   
  } else if( t192 & 0x40 ) {
    // we're in the middle third
    heatcolor.r = 255; // full red
    heatcolor.g = heatramp; // ramp up green
    heatcolor.b = 0; // no blue
   
  } else {
    // we're in the coolest third
    heatcolor.r = heatramp; // ramp up red
    heatcolor.g = 0; // no green
    heatcolor.b = 0; // no blue
  }
 
  return heatcolor;
}