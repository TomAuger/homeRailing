/*
IR.cpp

Base code for polling the IR transmit/receiver pair and getting
a proximity reading.

It works by first measuring the ambient light level as read by the
photoreceptor over a number of samples and deriving the average
ambientLevel for that cycle. Then we turn on the IR diode for a
number of samples and re-read the level on the photodiode.

The difference between these two readings (ambientLevel and irLevel)
can be interpreted as the "proximity" based on reflected IR light,
with lower values on the photoreceptor pin translating into
"closer" proximity.

An additional consideration is the noise that is introduced in the
system when the IR diode is on, whether this is due to a voltage
drop across the circuit or light from the diode leaking directly
into the photoreceptor.

To compensate for this, we can run a calibration cycle that calculates
the delta between the IR and non-IR readings at the photoreceptor.
This is generally pretty constant, so we can get the average delta
using the calibration cycle, and factor that into our proximity
calculation. Because there is always a little bit of fluctuation,
we can define a TOLERANCE constant that provides  bit of headroom
before a proximity event is triggered.


@TODO: refactor this into a Class, with a static collection of
IR Objects so we can calibrate and poll multiple IR transmit/receiver pairs.
*/

#include <arduino.h>

#define PIN_IR_PHOTODIODE_BASIS A6
#define PIN_IR_PHOTODIODE_1 A7
#define PIN_IR_TRANSMIT 2

#define SAMPLE_DELAY 700
#define SAMPLE_DEAD_TICKS 10
#define IR_SAMPLES 200
#define IR_PROXIMITY_TOLERANCE 5

#define SAMPLE_STATE_DEAD 0
#define SAMPLE_STATE_AMBIENT 1
#define SAMPLE_STATE_IR 2

int sampleState = SAMPLE_STATE_AMBIENT;
int sampleTick = 0;
unsigned long sampleLevel = 0;
unsigned long totalIRDelta = 0;

unsigned short irDelta = 0;
unsigned short ambientLevel = 0;
unsigned short irLevel = 0;


void scanIR(){
    if (SAMPLE_STATE_AMBIENT == sampleState){
      if (sampleTick < SAMPLE_DELAY){
        sampleLevel += analogRead(PIN_IR_PHOTODIODE_1);
        sampleTick++;
      } else {
        ambientLevel = sampleLevel / sampleTick;

        sampleLevel = 0;
        sampleTick = 0;

        sampleState = SAMPLE_STATE_DEAD;
      }
    }

    if (SAMPLE_STATE_DEAD == sampleState){
      if (sampleTick < SAMPLE_DEAD_TICKS){
        sampleTick++;
      } else {
        sampleTick = 0;

        sampleState = SAMPLE_STATE_IR;

        digitalWrite(PIN_IR_TRANSMIT, HIGH);
      }
    }

    if (SAMPLE_STATE_IR == sampleState){
      if (sampleTick < IR_SAMPLES){
        sampleLevel += analogRead(PIN_IR_PHOTODIODE_1);
        sampleTick++;
      } else {
        irLevel = sampleLevel / sampleTick;

        sampleLevel = 0;
        sampleTick = 0;
        sampleState = SAMPLE_STATE_AMBIENT;

        digitalWrite(PIN_IR_TRANSMIT, LOW);


        // Serial.print(ambientLevel);
        // Serial.print(" ");
        // Serial.print(irLevel);
        // Serial.print(" ");
        // Serial.println(constrain(abs(ambientLevel - irLevel), 0, 1024));
      }
    }
}

// Get the baseline difference reading between the IR being on vs off
void calibrateIR(){

  // Initialize the IR transmitter pin
  pinMode(PIN_IR_TRANSMIT, OUTPUT);
  digitalWrite(PIN_IR_TRANSMIT, LOW);

  // Do 10 complete cycles
  unsigned int baselineSamples = (SAMPLE_DELAY + SAMPLE_DEAD_TICKS + IR_SAMPLES);

  // Reset everything for kicks.
  sampleState = SAMPLE_STATE_AMBIENT;
  sampleTick = 0;
  sampleLevel = 0;
  totalIRDelta = 0;
  irDelta = 0;
  ambientLevel = 0;
  irLevel = 0;

  Serial.println("\n\nCalibrating IR Baseline...");

  for (int i = 0; i < 10; ++i){
    for (unsigned int baselineTick = 0; baselineTick <= baselineSamples; baselineTick++){
      scanIR();
    }

    totalIRDelta += abs(ambientLevel - irLevel);
  }

  irDelta = totalIRDelta / 10;
  Serial.println();
  Serial.println("Calibration complete.");
  Serial.print("IR Baseline Delta: ");
  Serial.println(irDelta);
}

unsigned short getIRProximity(){
  short proximity = abs(ambientLevel - irLevel) - irDelta;
  if (proximity < IR_PROXIMITY_TOLERANCE){
    return 0;
  }

  return (unsigned short) proximity;
}
