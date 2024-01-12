#include <Arduino.h>
#include <CuteBuzzerSounds.h>
#include <HomeSpan.h>
#include <ld2410.h>

// project's constants
#define BUZZER_PIN 21
#define BUTTON_PIN 4
#define RELAY_PIN 26

#define PROMIXITY_OPENING_CM 80

bool proximity_detected = false;

struct MotionSensor : Service::MotionSensor {  // Motion sensor

  SpanCharacteristic *movement;  // reference to the MotionDetected Characteristic
  ld2410 radar;
  uint32_t lastReading = 0;

  MotionSensor() : Service::MotionSensor() {
    movement = new Characteristic::MotionDetected(false);  // instantiate the MotionDetected Characteristic
                                                           // radar.debug(Serial); //Uncomment to show debug information from the library on the Serial Monitor. By default this does not show sensor reads as they are very frequent.
    Serial1.begin(256000, SERIAL_8N1, 17, 16);              // UART for monitoring the radar

    delay(500);
    LOG1(F("\nLD2410 radar sensor initialising: "));
    if (radar.begin(Serial1)) {
      LOG1(F("OK"));
    } else {
      LOG1(F("not connected"));
    }
  }  // end constructor

  void loop() {
    radar.read();

    if (radar.isConnected() && millis() - lastReading > 2000) {
      boolean motion = false;

      lastReading = millis();
      if (radar.presenceDetected()) {
        if (radar.stationaryTargetDetected()) {
          LOG1("Stationary target: %d\n", radar.stationaryTargetDistance());
          LOG1("cm energy:%d\n",radar.stationaryTargetEnergy());
          motion = true;
          proximity_detected = false;
        }
        if (radar.movingTargetDetected()) {
          LOG1("Moving target: %d\n", radar.movingTargetDistance());
          LOG1("cm energy: %d\n", radar.movingTargetEnergy());
          if (radar.movingTargetDistance() < PROMIXITY_OPENING_CM) {
            proximity_detected = true;
          } else {
            proximity_detected = false;
          }
          motion = true;
        }
      } else {
        WEBLOG("No target\n");
      }

      if (motion != movement->getVal()) {
        movement->setVal(motion);
        if (motion == true) {
          WEBLOG("Motion was detected\n");
        }
      }
    }
  }
};

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
  uint32_t lastOpening = 0;

  SecureLock() : Service::LockMechanism() {  // constructor() method

    current = new Characteristic::LockCurrentState(1);  // initial value of 1 means closed
    target = new Characteristic::LockTargetState(1);    // initial value of 1 means closed

    LOG1("Configuring Door LockMechanis\n");  // initialization message

    new SpanButton(BUTTON_PIN);
  }

  boolean update() {  // update() method

    if (target->getNewVal() == 0) {
      WEBLOG("Opening Door\n");
      current->setVal(0);
      digitalWrite(RELAY_PIN, HIGH);
      cute.play(11);
      delay(3000);
      target->setVal(1);
      digitalWrite(RELAY_PIN, LOW);
      current->setVal(1);
      cute.play(12);

      LOG1("Door Closed\n");
    } else {
      LOG1("Closing Door\n");
      current->setVal(1);
      cute.play(12);
      WEBLOG("Door Closed\n");
    }

    return (true);  // return true

  }  // update

  void loop() {
    if (proximity_detected && millis() - lastOpening > 7000) {
      lastOpening = millis();
      proximity_detected = false;

      target->setVal(0);
      update();
     // cute.play(10);
    }
  }

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
        //delay(2000);
        update();
        cute.play(11);
      } else if (pressType == SpanButton::LONG) {
        ESP.restart();
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
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  cute.init(BUZZER_PIN);

  homeSpan.enableWebLog(10, "pool.ntp.org", "UTC", "myLog");  // creates a web log on the URL /HomeSpan-[DEVICE-ID].local:[TCP-PORT]/myLog
  homeSpan.setLogLevel(1);
  homeSpan.setQRID("SPST");
  homeSpan.setPairingCode("04288230");
  homeSpan.enableOTA(false, true);
  homeSpan.begin();

  SPAN_ACCESSORY("Door Lock");
  new SecureLock();  // create 8-LED NeoPixel RGB Strand with full color control

  SPAN_ACCESSORY("Motion Sensor");
  new MotionSensor();
}
#define BUZZER_CHANNEL 0
void loop() {
  homeSpan.poll();
}
