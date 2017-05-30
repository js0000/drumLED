// Sound_to_light_with_control

#include <potentiometer.h>

/*
Sound to light example

segment display not implemented just yet.

This is an example program to test the controls of the sound to light project.

two potentiometers on 
pin A1 used for modulating the current mode
pin A2 used for setting the brighness 
Sound sensor is on A0

*/


#include <Adafruit_NeoPixel.h>
#define PIN 6
#include <SevenSegmentDisplay.h>
#include <button.h>
#include <Potentiometer.h>

#define COLOR_WHITE	0xFFFFFF
#define COLOR_RED	0xFF0000
#define COLOR_GREEN     0x00FF00
#define COLOR_BLUE	0x0000FF
#define COLOR_YELLOW	0xFFFF00
#define COLOR_CYAN 0x00FFFF
#define COLOR_MAGENTA 0xFF00FF
#define COLOR_SILVER	0xC0C0C0
#define COLOR_BROWN	0x8B4513

//define the input pins that are attached to each control
#define SEGMENT_DISPLAY_START_PIN 13
#define MODE_BUTTON_PIN 4
#define BRIGHTNESS_POT_PIN A2
  // mode control is the potentiometer control for each mode
#define MODE_CONTROL_POT_PIN A1


// Each contol or display has it's own class
SegmentDisplay segmentdisplay(SEGMENT_DISPLAY_START_PIN);
MyButton modeButton(MODE_BUTTON_PIN);
Potentiometer ControlPot(MODE_CONTROL_POT_PIN);
Potentiometer BrightnessContol(BRIGHTNESS_POT_PIN);  // potentiometr a2 that controls the brightness of the strip


// Using Adafruit library....
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);


int stripSetOneColor(uint32_t color);

int mode=0;
int maxModes=10;

int iCount =0;

// SETUPS

// the setup routine runs once when you press reset:
void setup() {   
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(9600);      // open the serial port at 9600 bps:    
  
  segmentdisplay.Display(0); 
}


#define COLOR_CYAN 0x00FFFF
#define COLOR_MAGENTA 0xFF00FF
#define COLOR_SILVER	0xC0C0C0

void loop() {

  switch (mode)
  {
     case 0:
        stripSetOneColor(COLOR_WHITE);
     break;    
     case 1:   
        stripSetOneColor(COLOR_RED);
     break;
     case 2:
        stripSetOneColor(COLOR_BLUE);
     break;
     case 3:
        stripSetOneColor(COLOR_GREEN);
     break;
     case 4:
        stripSetOneColor(COLOR_CYAN);
     break;
     case 5:
        stripSetOneColor(COLOR_MAGENTA);
     break;
     case 6:
        stripSetOneColor(COLOR_YELLOW);
     break;
     case 7:
        stripSetOneColor(COLOR_BROWN);
     break;
     case 9:
        rainbowStrip(ControlPot.Peek());    
//        Serial.println(" finished rainbowCycle ");
     break;
//     default:
//        Serial.println(" passing thru ");
  }

  static int count= 0;
  if ( modeButton.HasBeenPressed() == 1) 
  {  count++;
     if (count > 9) count = 0;
  }
  segmentdisplay.Display(count); 
  mode = count;
  
//  Serial.println("end loop ");
  
}


int incrementMode()
{
  mode=mode+1;
  if (mode == maxModes) 
  {
    mode =0;
  }
}

void setTheBrightness(int PotValue)
{
  static uint8_t brightness;
  static int maximum_pot_value =781;  // arbitrary maximum value for this potentiometer

  if (maximum_pot_value < PotValue)
    maximum_pot_value = PotValue;
    
  unsigned long tempBrightness = (PotValue *255.0)/maximum_pot_value;
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
  static int savedPotBValue = 0;
   
   int potBValue =  BrightnessContol.Peek();    

  if (savedPotBValue != potBValue)
  {
    savedPotBValue = potBValue;
    setTheBrightness(savedPotBValue);
    
  }
}


// Input a value 0 to 255 to get a color value.
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
  unsigned int potentiometerValue;
  
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
   for(i=0; i< strip.numPixels() ; i++) 
   {
      strip.setPixelColor(i, color);
   }
  strip.show();
}
