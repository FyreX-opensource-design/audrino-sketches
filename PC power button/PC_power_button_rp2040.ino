/*
 * Button latch with hold-to-off (RP2040 / Raspberry Pi Pico).
 *
 *   Short press (tap)     -> OUTPUT_PIN goes HIGH and stays HIGH
 *   Hold while on         -> after HOLD_MS, OUTPUT_PIN goes LOW
 *   Off button press      -> OUTPUT_PIN goes LOW if it is on
 *
 * Pin numbers are GPIO numbers (GP0, GP1, ...):
 *
 *   Silkscreen GP3  ->  use 3 in code
 *   Silkscreen GP5  ->  use 5 in code
 *   Silkscreen GP7  ->  use 7 in code
 *
 * Note: Pico GPIO is 3.3V only. Do not drive pins above 3.3V.
 */

const int BUTTON_PIN = 3;  // GP3
const int OFF_PIN = 5;     // GP5
const int OUTPUT_PIN = 7;  // GP7

// true  = button connects pin to GND when pressed (internal pull-up)
// false = button connects pin to 3.3V when pressed (internal pull-down)
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
  pinMode(pin, toGnd ? INPUT_PULLUP : INPUT_PULLDOWN);
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

  Serial.print("GP");
  Serial.print(OUTPUT_PIN);
  Serial.print(": ");
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
  while (!Serial && millis() < 3000) {
    // Wait for USB serial on Pico (up to 3s), then run anyway
  }

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
  Serial.println("GP7: LOW");
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
