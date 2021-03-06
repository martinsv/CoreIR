/*
*
* I-lap "transponder" compatible IR transmitter for RC timing system
*
* Type in a 7-digit ID and an alternate ID into the define fields before compiling.
* Note that using non-random ID numbers will increase your chances of encountering 
* another unit using the same ID.
* 
* !!!!IDs with repeated numbers may not register every pass. For best results don't repeat the same digit multiple times in a row!!!!
* Test ID code with system before using, if possible.
* 
*/
//CONFIGURABLE SECTION - SET TRANSPONDER ID
//Change transponder ID # by setting a different transponder number for tx_id
//WARNING: IDs set by CoreIR-Uplink tool will override these numbers
const long tx_id = 5118895;
const long tx_alt_id = 8901234;
const int easylap_id = 2;

// Enable debug info on serial output
//#define debug

// EasyRaceLapTimer support ---- TESTING
  #define easytimer

  
#ifdef easytimer
  const int easylap_on = 0;
#endif

//check which arduino board we are using and build accordingly
#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny167__) || defined(__AVR_ATtiny85__)
  //if using an attiny build with all defaults, don't define anything
#elif defined(__AVR_ATmega32U4__)
  //if using an arduino micro build with eeprom enabled and different LED pin
  #define atmega
  #define micro
#elif defined(__AVR_ATmega328p__) || defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega168__)
  //if using an atmega328p or similar build with eeprom enabled
  #define atmega
#else
  //define nothing
#endif

#if defined(easytimer)
  #include "Easytimer.h"
#endif

//TODO, set pins for attiny85 boards
#ifdef micro
  // Set up alternate ID jumper bridge, shift pins because micro is special and uses pin 5 for IR output
  #define bridgePinIn 14
  #define bridgePinOut 15
  // Change the status LED location to the proper pin for the atmega32U4
  const int ledPin = 17;
#else
  // Set up alternate ID jumper bridge using normal pins 5 and 6
  #define bridgePinIn 5
  #define bridgePinOut 6
  // Use the normal status LED
  const int ledPin = 13;
#endif

// Include eeprom library for usb connectivity and CoreIR-Uplink ID saving
#ifdef atmega
  #include <EEPROM.h>
  #include "saved.h"
#endif
String incomingString = "";   // for incoming serial data

// Include libraries for IR LED frequency, speed and encoding
#include "IRsnd.h"
#include "Encode.h"

// Initialize IR LED send code
IRsend irsend;

// Set up status LED
int ledState = LOW;
long interval = 0;
unsigned long previousMillis = 0; // Allow status LED to blink without sacraficing a timer

void setup() {
  #ifdef atmega
    // save the new transponder numbers in eeprom if they are not already there
    if (EEPROMReadlong(64) != tx_id) {
      long tx_id = EEPROMReadlong(64);
    }
    if (EEPROMReadlong(70) != easylap_id) {
      long tx_alt_id = EEPROMReadlong(70);
    }
  #endif

  // set up jumper bridge for alternate ID
  pinMode(bridgePinIn, INPUT);
  digitalWrite(bridgePinIn, HIGH);
  pinMode(bridgePinOut, OUTPUT);
  digitalWrite(bridgePinOut, LOW);

  #ifdef atmega
    Serial.begin(9600);
    #if defined(micro) & defined(debug)
      while (!Serial)
    #endif
  #endif

  // Generate timecode
  #ifdef debug
    Serial.print("Building code from: ");
  #endif
  if (easylap_on == 1){
    geteasylapcode();
  }
  else {
    if (digitalRead(bridgePinIn)) {
      #ifdef debug
        Serial.println("Main ID:");
        Serial.println(tx_id);
      #endif
      interval = 200; // Blink LED faster for regular id by default
      makeOutputCode(tx_id); // use alternate ID if unbridged
    } else {
      #ifdef debug
        Serial.println("Alternate ID: ");
        Serial.println(tx_alt_id);
      #endif
      interval = 1000; // Blink LED slower for alt id
      makeOutputCode(tx_alt_id); // use standard ID otherwise
    }
  }
  
  // Set up for blinking the status LED
  pinMode(ledPin, OUTPUT);
}

void loop() {
  #ifdef atmega
  //serial data stuff for coreir-uplink support
  // send data only when you receive data:
  if (easylap_on == 1) {
    if (Serial.available() > 0) {
      // read the incoming byte:
      incomingString = Serial.readString();
      //if asked, send back the current ID
      if (incomingString == "readID") {
        Serial.println(EEPROMReadlong(70));
        incomingString = "";
      }
      else {
      //if asked, set a new ID
        incomingString.remove(0,7);
        long idtolong = incomingString.toInt();
        EEPROMWritelong(70, idtolong);
        delayMicroseconds(3000);
        long easylap_id = EEPROMReadlong(70);
        Serial.println(easylap_id);
        incomingString = "";
        }
      }
    }
    else {
      if (Serial.available() > 0) {
      // read the incoming byte:
      incomingString = Serial.readString();
      //if asked, send back the current ID
      if (incomingString == "readID") {
        Serial.println(EEPROMReadlong(64));
        incomingString = "";
      }
      //if asked, set a new ID
      if (incomingString.length() > 7) {
        incomingString.remove(0,7);
        long idtolong = incomingString.toInt();
        EEPROMWritelong(64, idtolong);
        delayMicroseconds(3000);
        long tx_id = EEPROMReadlong(64);
        Serial.println(tx_id);
        incomingString = "";
        }
      }
    }
    #endif

    
  if (easylap_on == 1){
      irsend.sendRaw(buffer,NUM_BITS,38);
      delay(17 + random(0, 5));
    }
    else {
      //Send the IR signal, then wait the appropriate amount of time before re-sending
      irsend.sendRaw(outputcode, codeLen, khz);
      delay(7 + random(2, 5));
    }
  
  // -----Status LED blink code start -----
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
      // set the LED with the ledState of the variable:
      digitalWrite(ledPin, ledState);
    // -----LED blink code end -----
  }
  delayMicroseconds(500);
}
