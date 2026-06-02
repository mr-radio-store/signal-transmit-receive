#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- OLED setup ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- HX1838 input ----------
#define IR_PIN 2  // HX1838 OUT → Arduino D2 (interrupt)
volatile unsigned long lastTime = 0;
volatile unsigned long pulseInterval = 0;
volatile bool pulseDetected = false;

// ---------- Interrupt routine ----------
void measurePulse() {
  unsigned long now = micros();
  pulseInterval = now - lastTime;
  lastTime = now;
  pulseDetected = true;
}

void setup() {
  Serial.begin(9600);

  pinMode(IR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_PIN), measurePulse, CHANGE);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 not found"));
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println(F("No Signal"));
  display.display();
}

void loop() {
  if (pulseDetected) {
    pulseDetected = false;

    // Only accept valid pulses (ignore very long/short pulses)
    if (pulseInterval > 0 && pulseInterval < 5000) {
      float freq_kHz = 1e6 / (2.0 * pulseInterval) / 1000.0; // Convert to kHz

      // Serial debug
      Serial.print("Frequency: ");
      Serial.print(freq_kHz, 1);
      Serial.println(" kHz");

      // Display detected frequency
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 10);
      display.println(F("Detected"));

      display.setTextSize(2);
      // Center text
      String freqStr = String(freq_kHz, 1) + " kHz";
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(freqStr, 0, 0, &x1, &y1, &w, &h);
      int centerX = (SCREEN_WIDTH - w) / 2;
      display.setCursor(centerX, 40);
      display.println(freqStr);
      display.display();

    } else {
      // Invalid pulse → show "No Signal"
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 20);
      display.println(F("No Signal"));
      display.display();

      Serial.println("No signal detected");
    }
  }
}
