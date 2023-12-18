#include <Arduino.h>
#include <HomeSpan.h>
#include "Tone32.h"

#define BUZZER_PIN 5
#define BUTTON_PIN 4
#define RELAY_PIN 26

#define BUZZER_PWM_CHANNEL 0

struct SecureLock : Service::LockMechanism {
  SpanCharacteristic *current;  // reference to the Current Door State Characteristic
  SpanCharacteristic *target;   // reference to the Target Door State Characteristic

  SecureLock() : Service::LockMechanism() {  // constructor() method

    current = new Characteristic::LockCurrentState(1);  // initial value of 1 means closed
    target = new Characteristic::LockTargetState(1);    // initial value of 1 means closed

    Serial.print("Configuring Door LockMechanism");  // initialization message
    Serial.print("\n");

    new SpanButton(BUTTON_PIN);
  }

  boolean update() {  // update() method
    // see HAP Documentation for details on what each value represents

    if (target->getNewVal() == 0) {  // if the target-state value is set to 0, HomeKit is requesting the door to be in open position
      LOG1("Opening Door\n");

      current->setVal(0);  // set the current-state value to 0, which means "opening"
      LOG1("Current set to 0, opening the gate for 1 second\n");
      digitalWrite(RELAY_PIN, HIGH);  // close the contact on the relay
      delay(3000);                  // wait for a second
      LOG1("Door Opened\n");
      target->setVal(1);  // lock again
      LOG1("Target set to 1, closing the gate\n");
      digitalWrite(RELAY_PIN, LOW);  // open contact on the relay
      current->setVal(1);
      LOG1("Door Closed\n");
    } else {
      LOG1("Closing Door\n");
      current->setVal(1);  // set the current-state value to 1, which means "closing"
      LOG1("Door Closed\n");
    }

    return (true);  // return true

  }  // update

  // NEW!  Here is the button() method where all the PushButton actions are defined.   Take note of the signature, and use of the word "override"

  void button(int pin, int pressType) override {
    LOG1("Found button press on pin: ");  // always a good idea to log messages
    LOG1(pin);
    LOG1("  type: ");
    LOG1(pressType == SpanButton::LONG ? "LONG" : (pressType == SpanButton::SINGLE) ? "SINGLE"
                                                                                    : "DOUBLE");
    LOG1("\n");

    int newLevel;

    if (pin == BUTTON_PIN) {
      if (pressType == SpanButton::SINGLE) {  // if a SINGLE press of the power button...
        target->setVal(0);
        delay(2000);
        update();
      }
    }
  }
};

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    
    digitalWrite(RELAY_PIN, LOW);

    //ledcWriteTone(BUZZER_PWM_CHANNEL, 800);
    //delay(500);

    homeSpan.enableWebLog(10, "pool.ntp.org", "UTC", "myLog");  // creates a web log on the URL /HomeSpan-[DEVICE-ID].local:[TCP-PORT]/myLog
    homeSpan.setQRID("SPST");
    homeSpan.setPairingCode("04288230");

    homeSpan.begin(Category::Doors, "SecureLock");

    SPAN_ACCESSORY();  // create Bridge (note this sketch uses the SPAN_ACCESSORY() macro, introduced in v1.5.1 --- see the HomeSpan API Reference for details on this convenience macro)
    SPAN_ACCESSORY("Door Lock");

    new SecureLock();  // create 8-LED NeoPixel RGB Strand with full color control

}

void loop() {
  homeSpan.poll();
}
