#include <arduino.h>
#include "FastLED.h"

// Mode switching
#define PIN_MODE_BUTTON 13

// It would be nice to use an Enum here, but then how do you iterate over them?
#define MODE_LIGHT 0
#define MODE_PLAYGROUND 1
#define MODE_LASERS 2
#define MODE_OFF 3

#define NUM_MODES 4

int currentMode = 0;

// Limit maximum bightness for power or heat reasons
#define MAX_BRIGHTNESS 255
// Starting brightness for "light" mode
#define INIT_LIGHT_BRIGHTNESS 128
// Number of minutes before the demo modes timeout
#define PLAYGROUND_TIMEOUT_MINUTES 60
// How long before we register a button down as a "long press"
#define LONGPRESS_TIMEOUT_MILLIS 3000


bool modeButtonDown = false;
bool modeButtonLongpress = false;

unsigned short currentPlayground = 0;
unsigned short currentBrightness = INIT_LIGHT_BRIGHTNESS;
unsigned short lastBrightness = 0;
unsigned short currentLightHue = 50;
unsigned short currentLightSat = 128;


char brightnessIncrementDirection = 1;
char saturationIncrementDirection = 1;
unsigned long longpressTimeoutMillis = 0;
unsigned char calibrationMode = 0;
unsigned long currentModeTimeoutMillis = 0;


#define PIN_TOP_ACTUATOR_BUTTON 12
bool topActuatorButtonDown = false;

// FastLED Constants
#define LED_DATA_PIN    5
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    160
#define NUM_LEDS_PER_SEGMENT 32

CRGB leds[NUM_LEDS];

#define FRAMES_PER_SECOND  120

// Quick macro
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

//unsigned long tickMillis = 0;


void setup(){
  Serial.begin(9600);

  // calibrateIR();

  Serial.println("FastLED recovery delay...");
  delay(3000); // 3 second delay for recovery
  Serial.println("Ready.");

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // set master brightness control
  FastLED.setBrightness(MAX_BRIGHTNESS);
}




void displayBinary(unsigned short byte, const struct CRGB & color){

}

void debugLightMode(){
  Serial.print("hue: ");
  Serial.print(currentLightHue);
  Serial.print(" sat: ");
  Serial.print(currentLightSat);
  Serial.print(" brightness: ");
  Serial.println((currentBrightness/255.0) * MAX_BRIGHTNESS);
}

void setMode(int mode){
  currentMode = mode;

  Serial.print("New mode: ");
  Serial.println(currentMode);

  switch(currentMode){
    case MODE_LIGHT :
      // Disable timeouts
      currentModeTimeoutMillis = 0;

      // retrigger brightness
      lastBrightness = 0;
      FastLED.setBrightness(MAX_BRIGHTNESS);

      debugLightMode();
      break;

    case MODE_OFF :
      // Turn out the lights baby!
      FastLED.setBrightness(0);

      // Disable timeouts
      currentModeTimeoutMillis = 0;
      break;

    default :
      // Set the timeout for when we will switch back to default mode
      currentModeTimeoutMillis = millis() + (PLAYGROUND_TIMEOUT_MINUTES * 60.0 * 1000);
      FastLED.setBrightness(MAX_BRIGHTNESS);
  }
}

void nextMode(){
  setMode((currentMode + 1) % NUM_MODES);
}


// Light patterns ===================================================================
// Playground patterns
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void rainbow(){
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void confetti(){
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon(){
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm(){
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle(){
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void crossFadePalettes(){

}

void quickCycle(){
  // quick hue cycle to let us know we're in Hue Set mode
  unsigned short hue = 0;

  while (hue < 256){
    fill_solid(leds, NUM_LEDS, CHSV(hue, 255, MAX_BRIGHTNESS));
    FastLED.show();

    hue += 10;
    delay(10);
  }
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, confetti, sinelon, juggle, bpm };

void nextPattern(){
  switch(currentMode){
    case MODE_LASERS :

      break;

    case MODE_PLAYGROUND :
      // add one to the current pattern number, and wrap around at the end
      currentPlayground = (currentPlayground + 1) % ARRAY_SIZE( gPatterns);
      Serial.print("Next Playground pattern: ");
      Serial.println(currentPlayground);
      break;
  }
}




// The LOOP ==================================================================
void loop(){
  // scanIR();
  //
  // if (millis() - tickMillis > 500){
  //   Serial.print("Proximity: ");
  //   Serial.println(getIRProximity());
  //
  //   tickMillis = millis();
  // }




  // MODE Button DOWN
  if (digitalRead(PIN_MODE_BUTTON)){
    // First poll?
    if (! modeButtonDown){
      Serial.println("        [Mode Button pressed]");
      modeButtonDown = true;

      // ..and setup the longpress timeout
      longpressTimeoutMillis = millis() + LONGPRESS_TIMEOUT_MILLIS;
    }

    // MODE Button LONGPRESS
    if (! modeButtonLongpress && millis() > longpressTimeoutMillis){
      modeButtonLongpress = true;

      switch(currentMode){
        // In Light mode, we're going to set hue/sat
        case MODE_LIGHT :
            // calibrationMode
            // 0: brightness
            // 1: hue
            // 2: sat
            calibrationMode = 1;

            Serial.println("Calibration Mode: 1");

            // Provide a little user feedback...
            quickCycle();
            debugLightMode();

            break;
          }
      }
  }

  // MODE Button UP
  else {
    // Switch modes on release (but not when in calibration mode!)
    if (modeButtonDown){
      // Don't switch modes or do anything on button release if
      // we just entered Longpress mode.
      if (modeButtonLongpress){
        Serial.println("        [Longpress released]");

        modeButtonLongpress = false;
      } else {
        // If we're in calibration mode, then the MODE button
        // switches calibration mode...
        if (calibrationMode > 0){
          calibrationMode = (calibrationMode + 1) % 3;
          Serial.print("Calibration Mode: ");
          Serial.println(calibrationMode);

          quickCycle();

          // Reset lastBrightness so we get an update even when calibration mode
          // is set to 0
          lastBrightness = 0;
        }
        // ...otherwise switch Modes.
        else {
          nextMode();
        }
      }
    }

      // Set this last so we can do things once on button release
      modeButtonDown = false;
  }

  // Do different things with the actuator button depending on the mode
  // ACTUATOR button DOWN
  if (digitalRead(PIN_TOP_ACTUATOR_BUTTON)){
    switch(currentMode){
      case MODE_LIGHT :
        switch (calibrationMode){
          case 1 :
            currentLightHue = (currentLightHue + 1) % 256;
            break;

          case 2 :
            currentLightSat = constrain(currentLightSat + saturationIncrementDirection, 0, 255);
            break;

          default :
            currentBrightness = constrain(currentBrightness + brightnessIncrementDirection, 0, 255);
            break;
        }

        break;

      case MODE_PLAYGROUND :
        if (! topActuatorButtonDown){
          nextPattern();
        }
        break;
    }

    // We can use this to differentiate from "buttonDown" vs. "buttonPress"
    topActuatorButtonDown = true;
  }

  // ACTUATOR button UP
  else {
    // Maybe do something when we release
    if (topActuatorButtonDown){
      switch(currentMode){
        case MODE_LIGHT :
          switch(calibrationMode){
            case 0 :
              // Switch from dimming to brightnening on release
              brightnessIncrementDirection = - (brightnessIncrementDirection);
              break;

            // Hue just rotates through the spectrum.

            case 2 :
              // Switch from increasing to decreasing saturation
              saturationIncrementDirection = - (saturationIncrementDirection);
              break;
          }

          debugLightMode();
          break;

        case MODE_OFF :

          break;

      }
    }


    topActuatorButtonDown = false;
  }


  // If we're in one of the showoff modes, check the timeout
  if (currentModeTimeoutMillis > 0 && millis() > currentModeTimeoutMillis){
    setMode(MODE_LIGHT);
  }

  // Handle the Loop!
  switch(currentMode){
    case MODE_LIGHT :
      // There's no need to change the LEDs array if the brightness hasn't changed
      // Though if we're in calibration mode, just redraw it cause I don't want
      // to clutter memory any further
      if (lastBrightness != currentBrightness || calibrationMode > 0){
        // Cap the brightness to the max brightness setting
        unsigned short brightness = (currentBrightness/255.0) * MAX_BRIGHTNESS;

        fill_solid(leds, NUM_LEDS, CHSV(currentLightHue, currentLightSat, brightness));
        lastBrightness = currentBrightness;
      }
      break;
    case MODE_LASERS :

      break;
    case MODE_PLAYGROUND :
        gPatterns[currentPlayground]();
      break;
  }


  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND);

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  // EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}
