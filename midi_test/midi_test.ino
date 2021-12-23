/*
  Based on Sketch built by Gustavo Silveira (aka Music Nerd)and Dolce Wang
  Modified by Vishal Bhat

  This code is only for Arduinos that use ATmega328 (like Uno, Mega, Nano, Beetle...)

*/



// LIBRARIES

#include <MIDI.h> // by Francois Best
#include <Adafruit_ADS1X15.h>
#include <MPR121.h>
#include <MPR121_Datastream.h>
#include <Wire.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
MIDI_CREATE_DEFAULT_INSTANCE(); 

#define NO_OF_POTENTIOMETERS 9   //6
#define NO_OF_PADS 12
#define ADS_2 2
#define ADS_1 1
#define ADS_0 0
#define MPR121_ADDR 0x5A
#define MPR121_INT 3

// BUTTONS
const bool MPR121_DATASTREAM_ENABLE = false;
const int NButtons = 12; //***  total number of push buttons
const int buttonPin[NButtons] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; //*** MPR121 electrodes
                                        //** Button NOTE will go up chromatically.  ie. if button is digi pin 2, C; Pin 3, C#; Pin 3, D; etc
                                   
int buttonCState[NButtons] = {};        // stores the button current value
int buttonPState[NButtons] = {};        // stores the button previous value


// debounce
unsigned long lastDebounceTime[NButtons] = {0};  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    //* the debounce time; increase if the output flickers

// POTENTIOMETERS
const int NPots = NO_OF_POTENTIOMETERS; //*** total numbers of pots (slide & rotary)
//int16_t ads_0, ads_1, ads_2;
const int potPin[NPots] = {A5, A4, A3, A2, A1, A0, ADS_2, ADS_1, ADS_0}; //*** Analog pins of each pot connected straight to the Arduino i.e 4 pots, {A3, A2, A1, A0};
                                          // have nothing in the array if 0 pots {}

int potCState[NPots] = {0}; // Current state of the pot; delete 0 if 0 pots
int potPState[NPots] = {0}; // Previous state of the pot; delete 0 if 0 pots
int potVar = 0; // Difference between the current and previous state of the pot

int midiCState[NPots] = {0}; // Current state of the midi value; delete 0 if 0 pots
int midiPState[NPots] = {0}; // Previous state of the midi value; delete 0 if 0 pots

const int TIMEOUT = 300; //** Amount of time the potentiometer will be read after it exceeds the varThreshold
const int varThreshold = 10; //** Threshold for the potentiometer signal variation
boolean potMoving = true; // If the potentiometer is moving
unsigned long PTime[NPots] = {0}; // Previously stored time; delete 0 if 0 pots
unsigned long timer[NPots] = {0}; // Stores the time that has elapsed since the timer was reset; delete 0 if 0 pots


// MIDI
byte midiCh = 1; //** MIDI channel to be used; You can add more if you need to reorganize or have a billion buttons/pots
byte note = 36; //** First note to be used for digital buttons, then go up chromatically in scale according to the sequence in your "buttonPin" array
                // you can look up on a Midi Note chart; 36=C2; 60=Middle C
byte cc = 1; //** First MIDI CC to be used for pots on Analog Pins in order of the "potPin" array; then goes up by 1

// SETUP
void setup() { 

  Serial.begin(115200); //**  Baud Rate 31250 for MIDI class compliant jack | 115200 for Hairless MIDI

  //ads1115 parameters
  ads.setGain(GAIN_TWOTHIRDS);
  
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  if(!MPR121.begin(MPR121_ADDR))
  {
    Serial.println("error setting up MPR121");
    while(1);
  }
  
  // Buttons
  MPR121.setInterruptPin(MPR121_INT);

  if (MPR121_DATASTREAM_ENABLE) 
  {
    MPR121.restoreSavedThresholds();
    MPR121_Datastream.begin(&Serial);
  } 
  else 
  {
    MPR121.setTouchThreshold(40);  // this is the touch threshold - setting it low makes it more like a proximity trigger, default value is 40 for touch
    MPR121.setReleaseThreshold(20);  // this is the release threshold - must ALWAYS be smaller than the touch threshold, default value is 20 for touch
  }

  MPR121.setFFI(FFI_10);
  MPR121.setSFI(SFI_10);
  MPR121.setGlobalCDT(CDT_4US);  // reasonable for larger capacitances
  
  digitalWrite(LED_BUILTIN, HIGH);  // switch on user LED while auto calibrating electrodes
  delay(1000);
  MPR121.autoSetElectrodes();  // autoset all electrode settings
  digitalWrite(LED_BUILTIN, LOW);
  
  // Initialize buttons with pull up resistors
//  for (int i = 0; i < NButtons; i++) {
//    pinMode(buttonPin[i], INPUT_PULLUP);
//  }

}

//Main LOOP
void loop() 
{

  Serial.println("Start of Loop");
  buttons();
  potentiometers();

}


// BUTTONS
void buttons() {
  MPR121.updateAll();
  for (int i = 0; i < NButtons; i++) {

    buttonCState[i] = MPR121.isNewTouch(buttonPin[i]); // read state from MPR121
    //buttonCState[i] = digitalRead(buttonPin[i]);  // read pins from arduino
    
//    if ((millis() - lastDebounceTime[i]) > debounceDelay) 
//    {
//      if (buttonPState[i] != buttonCState[i]) 
//      {
//        lastDebounceTime[i] = millis();
        if (MPR121.isNewTouch(buttonPin[i]))
        { // Sends the MIDI note ON based on a touch sensed on the particular MPR121 electrode
          MIDI.sendNoteOn(note + i, 127, midiCh); // note, velocity, channel
        }
        else if(MPR121.isNewRelease(buttonPin[i]))
        { // Sends the MIDI note OFF based on a release sensed on the particular MPR121 electrode
          MIDI.sendNoteOn(note + i, 0, midiCh); // note, velocity, channel
        }
        buttonPState[i] = buttonCState[i];
//      }
//    }
  }
  if (MPR121_DATASTREAM_ENABLE) 
  {
    MPR121_Datastream.update();
  }
}


// POTENTIOMETERS
void potentiometers() {


  for (int i = 0; i < NPots; i++) { // Loops through all the potentiometers

    if(i< 6)
    {
      potCState[i] = analogRead(potPin[i]); // reads the pins from arduino
      midiCState[i] = map(potCState[i], 0, 1023, 127, 0); // Maps the reading of the potCState to a value usable in midi
    }
    else if ( i >= 6 && i <= NPots)
    {
      potCState[i] = ads.readADC_SingleEnded(potPin[i]);
      midiCState[i] = map(potCState[i], 0, 26720, 127, 0); // Maps the reading of the potCState to a value usable in midi
    }
    //potCState[i] = analogRead(potPin[i]); // reads the pins from arduino

    //midiCState[i] = map(potCState[i], 0, 1023, 127, 0); // Maps the reading of the potCState to a value usable in midi

    potVar = abs(potCState[i] - potPState[i]); // Calculates the absolute value between the difference between the current and previous state of the pot

    if (potVar > varThreshold) { // Opens the gate if the potentiometer variation is greater than the threshold
      PTime[i] = millis(); // Stores the previous time
    }

    timer[i] = millis() - PTime[i]; // Resets the timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) { // If the timer is less than the maximum allowed time it means that the potentiometer is still moving
      potMoving = true;
    }
    else {
      potMoving = false;
    }

    if (potMoving == true) { // If the potentiometer is still moving, send the change control
      if (midiPState[i] != midiCState[i]) {

        // Sends the MIDI CC 
        MIDI.sendControlChange(cc + i, midiCState[i], midiCh); // cc number, cc value, midi channel
            

        potPState[i] = potCState[i]; // Stores the current reading of the potentiometer to compare with the next
        midiPState[i] = midiCState[i];
      }
    }
  }
}
