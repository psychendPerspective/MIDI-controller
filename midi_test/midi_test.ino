/*
  Based on Sketch built by Gustavo Silveira (aka Music Nerd)
  Modified by Dolce Wang

  This code is only for Arduinos that use ATmega328 (like Uno, Mega, Nano, Beetle...)

*/


// Change values with //*** 


// LIBRARIES

#include <MIDI.h> // by Francois Best
MIDI_CREATE_DEFAULT_INSTANCE(); 


// BUTTONS
const int NButtons = 4; //***  total number of push buttons
const int buttonPin[NButtons] = {2, 3, 5, 7}; //*** define digital pins connected from button to Arduino; (ie {10, 16, 14, 15, 6, 7, 8, 9, 2, 3, 4, 5}; 12 buttons)
                                        //** Button NOTE will go up chromatically.  ie. if button is digi pin 2, C; Pin 3, C#; Pin 3, D; etc
                                   
int buttonCState[NButtons] = {};        // stores the button current value
int buttonPState[NButtons] = {};        // stores the button previous value


// debounce
unsigned long lastDebounceTime[NButtons] = {0};  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    //* the debounce time; increase if the output flickers

// POTENTIOMETERS
const int NPots = 6; //*** total numbers of pots (slide & rotary)
const int potPin[NPots] = {A5, A4, A3, A2, A1, A0}; //*** Analog pins of each pot connected straight to the Arduino i.e 4 pots, {A3, A2, A1, A0};
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

  
  // Buttons
  // Initialize buttons with pull up resistors
  for (int i = 0; i < NButtons; i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

}


// LOOP
void loop() {

  buttons();
  potentiometers();

}


// BUTTONS
void buttons() {

  for (int i = 0; i < NButtons; i++) {

    buttonCState[i] = digitalRead(buttonPin[i]);  // read pins from arduino


    if ((millis() - lastDebounceTime[i]) > debounceDelay) {

      if (buttonPState[i] != buttonCState[i]) {
        lastDebounceTime[i] = millis();

        if (buttonCState[i] == LOW) {

          // Sends the MIDI note ON accordingly to the chosen board

MIDI.sendNoteOn(note + i, 127, midiCh); // note, velocity, channel


        }
        else {

          // Sends the MIDI note OFF accordingly to the chosen board

MIDI.sendNoteOn(note + i, 0, midiCh); // note, velocity, channel

        }
        buttonPState[i] = buttonCState[i];
      }
    }
  }
}


// POTENTIOMETERS
void potentiometers() {


  for (int i = 0; i < NPots; i++) { // Loops through all the potentiometers

    potCState[i] = analogRead(potPin[i]); // reads the pins from arduino

    midiCState[i] = map(potCState[i], 0, 1023, 0, 127); // Maps the reading of the potCState to a value usable in midi

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
