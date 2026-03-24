/*
 * Swarm Defense Trainer — Trigger Button
 * 
 * Upload this sketch to an Arduino Nano (or any Arduino board).
 * 
 * Wiring:
 *   - Connect a momentary push button between pin D2 and GND
 *   - The internal pull-up resistor is used (no external resistor needed)
 * 
 * Protocol:
 *   - When the button is pressed, sends 'F' over serial at 115200 baud
 *   - The UE5 HardwareTrigger plugin reads this byte to fire the weapon
 *   - 100ms debounce prevents multiple fires from a single press
 * 
 * LED Feedback:
 *   - Built-in LED (pin 13) lights up when the button is pressed
 */

const int TRIGGER_PIN = 2;
const int LED_PIN = 13;
const unsigned long DEBOUNCE_MS = 100;

unsigned long lastFireTime = 0;

void setup() {
    Serial.begin(115200);
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void loop() {
    // Button is active LOW (pulled up internally, grounded when pressed)
    if (digitalRead(TRIGGER_PIN) == LOW) {
        unsigned long now = millis();
        
        // Debounce check
        if (now - lastFireTime >= DEBOUNCE_MS) {
            Serial.write('F');  // Send fire signal
            lastFireTime = now;
            
            // Visual feedback
            digitalWrite(LED_PIN, HIGH);
        }
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}
