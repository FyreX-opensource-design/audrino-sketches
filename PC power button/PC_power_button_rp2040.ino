/*
 * PC power button controller (RP2040 / Raspberry Pi Pico).
 *
 *   Short press GP3             -> GP7 latches HIGH if it was off
 *   Short press GP3 while GP7 on -> pulse GP2 HIGH briefly (PC power switch)
 *   Hold GP3 while GP7 is HIGH  -> after HOLD_MS, GP7 goes LOW
 *   Off input on GP5            -> GP7 goes LOW if it is on
 *
 * Note: Pico GPIO is 3.3V only. Do not drive pins above 3.3V.
 */

const int BUTTON_PIN = 3;  // GP3
const int PULSE_PIN = 2;   // GP2
const int OFF_PIN = 5;     // GP5
const int OUTPUT_PIN = 7;  // GP7

// true  = button connects pin to GND when pressed (internal pull-up)
// false = button connects pin to 3.3V when pressed (internal pull-down)
const bool BUTTON_TO_GND = true;
const bool OFF_TO_GND = true;

const unsigned long HOLD_MS = 2000;
const unsigned long PULSE_MS = 250;
const unsigned long DEBOUNCE_MS = 25;
const unsigned long OFF_DEBOUNCE_MS = 100;

bool outputOn = false;
bool lastReading = false;
bool debouncedPressed = false;
bool lastDebouncedPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long pressStartTime = 0;
bool latchedOnThisPress = false;
bool holdOffThisPress = false;

bool offLastReading = false;
bool offDebouncedPressed = false;
bool lastOffDebouncedPressed = false;
unsigned long offDebounceTime = 0;

bool pulseActive = false;
unsigned long pulseEndTime = 0;

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

void startPulse() {
  digitalWrite(PULSE_PIN, HIGH);
  pulseActive = true;
  pulseEndTime = millis() + PULSE_MS;

  Serial.print("GP");
  Serial.print(PULSE_PIN);
  Serial.print(": pulse ");
  Serial.print(PULSE_MS);
  Serial.println(" ms");
}

void updatePulse() {
  if (pulseActive && millis() >= pulseEndTime) {
    digitalWrite(PULSE_PIN, LOW);
    pulseActive = false;

    Serial.print("GP");
    Serial.print(PULSE_PIN);
    Serial.println(": LOW");
  }
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
  pinMode(PULSE_PIN, OUTPUT);
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(PULSE_PIN, LOW);
  digitalWrite(OUTPUT_PIN, LOW);

  lastReading = readInputPressed(BUTTON_PIN, BUTTON_TO_GND);
  debouncedPressed = lastReading;
  lastDebouncedPressed = debouncedPressed;
  offLastReading = readInputPressed(OFF_PIN, OFF_TO_GND);
  offDebouncedPressed = offLastReading;
  lastOffDebouncedPressed = offDebouncedPressed;

  Serial.println("Ready v2. Short press latches GP7; short press while on = GP2 pulse.");
  Serial.println("GP2: LOW");
  Serial.println("GP7: LOW");
}

void loop() {
  updateDebouncedButton();
  updateDebouncedOffButton();
  updatePulse();

  if (offDebouncedPressed && !lastOffDebouncedPressed && outputOn) {
    setOutput(false, "off button");
  }

  if (debouncedPressed && !lastDebouncedPressed) {
    latchedOnThisPress = false;
    holdOffThisPress = false;
    pressStartTime = millis();
    Serial.print("GP3: pressed (GP7 ");
    Serial.print(outputOn ? "on" : "off");
    Serial.println(")");
    if (!outputOn) {
      setOutput(true, "short press");
      latchedOnThisPress = true;
    }
  }

  if (debouncedPressed && outputOn && !holdOffThisPress) {
    if (millis() - pressStartTime >= HOLD_MS) {
      setOutput(false, "held to turn off");
      holdOffThisPress = true;
    }
  }

  if (!debouncedPressed && lastDebouncedPressed) {
    unsigned long pressDuration = millis() - pressStartTime;
    bool shortPress = pressDuration < HOLD_MS;
    bool shouldPulse = shortPress && outputOn && !latchedOnThisPress && !holdOffThisPress;

    Serial.print("GP3: released (");
    Serial.print(pressDuration);
    Serial.print(" ms) outputOn=");
    Serial.print(outputOn ? 1 : 0);
    Serial.print(" latched=");
    Serial.print(latchedOnThisPress ? 1 : 0);
    Serial.print(" holdOff=");
    Serial.println(holdOffThisPress ? 1 : 0);

    if (shouldPulse) {
      startPulse();
    } else if (shortPress && latchedOnThisPress) {
      Serial.println("GP2: pulse skipped (this press latched GP7 on)");
    } else if (shortPress && holdOffThisPress) {
      Serial.println("GP2: pulse skipped (held to turn off)");
    } else if (shortPress && !outputOn) {
      Serial.println("GP2: pulse skipped (GP7 is off)");
    } else {
      Serial.println("GP2: pulse skipped (long press)");
    }
  }

  lastDebouncedPressed = debouncedPressed;
  lastOffDebouncedPressed = offDebouncedPressed;
}
