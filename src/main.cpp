#include <Arduino.h>
#include <HomeSpan.h>

#define BUTTON_PIN 4
#define RELAY_PIN 26

/**
 * SecureLock class that extends Service::LockMechanism to implement a secure
 * door lock using HomeSpan.
 *
 * Handles setting lock current/target state characteristics, responding to
 * button presses to trigger lock state changes, and updating lock state.
 */
struct SecureLock : Service::LockMechanism {
  SpanCharacteristic *current;
  SpanCharacteristic *target;

  SecureLock() : Service::LockMechanism() {  // constructor() method

    current = new Characteristic::LockCurrentState(1);  // initial value of 1 means closed
    target = new Characteristic::LockTargetState(1);    // initial value of 1 means closed

    Serial.print("Configuring Door LockMechanism");  // initialization message
    Serial.print("\n");

    new SpanButton(BUTTON_PIN);
  }

  boolean update() {  // update() method

    if (target->getNewVal() == 0) {
      LOG1("Opening Door\n");
      current->setVal(0);
      digitalWrite(RELAY_PIN, HIGH);
      delay(3000);
      target->setVal(1);
      digitalWrite(RELAY_PIN, LOW);
      current->setVal(1);
      LOG1("Door Closed\n");
    } else {
      LOG1("Closing Door\n");
      current->setVal(1);
      LOG1("Door Closed\n");
    }

    return (true);  // return true

  }  // update

  void button(int pin, int pressType) override {
    LOG1("Found button press on pin: ");
    LOG1(pin);
    LOG1("  type: ");
    LOG1(pressType == SpanButton::LONG ? "LONG" : (pressType == SpanButton::SINGLE) ? "SINGLE"
                                                                                    : "DOUBLE");
    LOG1("\n");

    int newLevel;

    if (pin == BUTTON_PIN) {
      if (pressType == SpanButton::SINGLE) {
        target->setVal(0);
        delay(2000);
        update();
      }
    }
  }
};

/**
 * @brief Set up initial state and register accessories with HomeSpan
 *
 * This function initializes the serial connection, configures the
 * relay pin, and registers the SecureLock accessory with HomeSpan.
 * It also enables web logging and sets the pairing code.
 */
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

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
