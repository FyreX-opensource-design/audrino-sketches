/*
 * Button latch with hold-to-off.
 *
 *   Short press (tap)     -> D7 goes HIGH and stays HIGH
 *   Hold while D7 is HIGH -> after HOLD_MS, D7 goes LOW
 *   Off button press      -> D7 goes LOW if it is on
 *
 * Pin numbers in code are the *digital* pin numbers, not always the silkscreen label:
 *
 *   Silkscreen D0-D13  ->  use 0-13 in code (D3 = 3, D7 = 7)
 *   Silkscreen A0-A5   ->  use A0-A5 or 14-19 (A3 = A3 = 17)
 *   Silkscreen A6-A7   ->  analog only, cannot use digitalRead/digitalWrite
 *
 * Wiring (Elegoo / Arduino Nano):
 *   D7 -> load (LED, relay, etc.)
 */

const int BUTTON_PIN = 3;  // silkscreen D3; use A3 if wired to analog header A3
const int OFF_PIN = 5;     // silkscreen D5
const int OUTPUT_PIN = 7;  // silkscreen D7

// true  = button connects pin to GND when pressed (internal pull-up)
// false = button connects pin to 5V when pressed (add 10k pulldown from pin to GND)
const bool BUTTON_TO_GND = true;
const bool OFF_TO_GND = true;

const unsigned long HOLD_MS = 2000;
const unsigned long DEBOUNCE_MS = 50;
const unsigned long OFF_DEBOUNCE_MS = 100;

bool outputOn = false;
bool lastReading = false;
bool debouncedPressed = false;
bool lastDebouncedPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long holdStartTime = 0;
bool holdOffTriggered = false;

bool offLastReading = false;
bool offDebouncedPressed = false;
bool lastOffDebouncedPressed = false;
unsigned long offDebounceTime = 0;

void configureInputPin(int pin, bool toGnd) {
  pinMode(pin, toGnd ? INPUT_PULLUP : INPUT);
}

bool readInputPressed(int pin, bool toGnd) {
  return toGnd ? (digitalRead(pin) == LOW) : (digitalRead(pin) == HIGH);
}

void setOutput(bool on, const char* reason) {
  if (outputOn == on) {
    return;
  }

  outputOn = on;
  digitalWrite(OUTPUT_PIN, on ? HIGH : LOW);

  Serial.print("D7 (pin ");
  Serial.print(OUTPUT_PIN);
  Serial.print("): ");
  Serial.print(on ? "HIGH" : "LOW");
  Serial.print(" (");
  Serial.print(reason);
  Serial.println(")");
}

void updateDebouncedButton() {
  bool reading = readInputPressed(BUTTON_PIN, BUTTON_TO_GND);

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime >= DEBOUNCE_MS) {
    debouncedPressed = reading;
  }

  lastReading = reading;
}

void updateDebouncedOffButton() {
  bool reading = readInputPressed(OFF_PIN, OFF_TO_GND);

  if (reading != offLastReading) {
    offDebounceTime = millis();
  }

  if (millis() - offDebounceTime >= OFF_DEBOUNCE_MS) {
    offDebouncedPressed = reading;
  }

  offLastReading = reading;
}

void setup() {
  Serial.begin(9600);

  configureInputPin(BUTTON_PIN, BUTTON_TO_GND);
  configureInputPin(OFF_PIN, OFF_TO_GND);
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);

  lastReading = readInputPressed(BUTTON_PIN, BUTTON_TO_GND);
  debouncedPressed = lastReading;
  lastDebouncedPressed = debouncedPressed;
  offLastReading = readInputPressed(OFF_PIN, OFF_TO_GND);
  offDebouncedPressed = offLastReading;
  lastOffDebouncedPressed = offDebouncedPressed;

  Serial.println("Ready. Short press = on, hold 2s while on = off, off button = off.");
  Serial.println("D7 (pin 7): LOW");
}

void loop() {
  updateDebouncedButton();
  updateDebouncedOffButton();

  if (offDebouncedPressed && !lastOffDebouncedPressed && outputOn) {
    setOutput(false, "off button");
    holdOffTriggered = true;
  }

  if (debouncedPressed && !lastDebouncedPressed) {
    if (!outputOn) {
      setOutput(true, "short press");
    }
    holdStartTime = millis();
    holdOffTriggered = false;
  }

  if (debouncedPressed && outputOn && !holdOffTriggered) {
    if (millis() - holdStartTime >= HOLD_MS) {
      setOutput(false, "held to turn off");
      holdOffTriggered = true;
    }
  }

  if (!debouncedPressed) {
    holdOffTriggered = false;
  }

  lastDebouncedPressed = debouncedPressed;
  lastOffDebouncedPressed = offDebouncedPressed;
}
