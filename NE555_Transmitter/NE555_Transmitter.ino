/* 
  Detect IR Signal from NE555 + IR LED via HX1838 Receiver
  Display frequency and status on ST7735 TFT

  Wire Connection
NE555 Transmitter (Astable Mode)
NE555 Pin	Connects to Arduino/Other	Notes
1 GND	Arduino GND	Ground
2 TRIG	Pin 6 (feedback for astable)	Connect to THR
3 OUT	220 Ω resistor → IR LED anode (+)	Pulse output drives IR LED
4 RESET	+5 V	Disable reset
5 CTRL	0.01 µF capacitor → GND	Optional for stability
6 THR	Pin 2 (feedback)	Connect to TRIG
7 DISCH	Timing resistor → +5 V	For astable RC timing
8 VCC	+5 V Arduino	Power

ST7735 TFT (SPI)
TFT Pin	Connects to Arduino	Notes
VCC	5 V or 3.3 V	Backlight & power
GND	GND	Ground
CS	D10	Chip select (matches code)
DC	D8	Data/Command
RST	D9	Reset
MOSI	D11	SPI data out
SCK	D13	SPI clock

C-05 Bluetooth
HC-05 Pin      Arduino Pin
---------------------------
TXD            → Arduino RX (D0) *
RXD            → Arduino TX (D1) *
VCC            → 5V
GND            → GND
*/


#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ---------- TFT pins ----------
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// ---------- IR / NE555 pins ----------
const int irPin = 2;   // IR receiver output
const int nePin = 3;   // NE555 control pin

// ---------- Bluetooth ----------
String btCommand = "";
bool transmitterOn = false;

// ---------- Frequency measurement ----------
unsigned long highTime, lowTime, period;
float frequency = 0;

// ---------- TFT layout ----------
const int titleY = 0;
const int currentY = 25;
const int graphY = 60;
const int graphHeight = 60;
const int screenWidth = 128;

int graphX = 0;   // horizontal scroll position

void setup() {
  Serial.begin(9600);          
  pinMode(irPin, INPUT);
  pinMode(nePin, OUTPUT);
  digitalWrite(nePin, LOW);    

  // Initialize TFT
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, titleY);
  tft.println("IR Freq Detector");

  drawGraphFrame();
  showOffStatus();
}

void loop() {
  readBluetoothCommand();

  if (btCommand.equalsIgnoreCase("ON")) {
    transmitterOn = true;
    digitalWrite(nePin, HIGH);
    clearDataArea();
    drawGraphFrame();
    Serial.println("NE555 Transmitter ON");
    btCommand = "";
  } 
  else if (btCommand.equalsIgnoreCase("OFF")) {
    transmitterOn = false;
    digitalWrite(nePin, LOW);
    showOffStatus();
    Serial.println("NE555 Transmitter OFF");
    btCommand = "";
  }

  if (transmitterOn) {
    measureAndDisplayFrequency();
  }

  delay(300);
}

// ================= Helper Functions =================

void readBluetoothCommand() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') continue;
    btCommand += c;
  }
  btCommand.trim();
}

void clearDataArea() {
  tft.fillRect(0, currentY, screenWidth, graphY - currentY, ST77XX_BLACK);
  tft.fillRect(0, graphY, screenWidth, graphHeight, ST77XX_BLACK);
}

void showOffStatus() {
  clearDataArea();
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(40, currentY + 20);
  tft.println("OFF");
}

void drawGraphFrame() {
  tft.drawRect(0, graphY, screenWidth, graphHeight, ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, graphY - 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Freq Graph");
}

void measureAndDisplayFrequency() {
  highTime = pulseIn(irPin, HIGH, 2000000);
  lowTime  = pulseIn(irPin, LOW, 2000000);
  period = highTime + lowTime;

  if (period > 0) {
    frequency = 1000000.0 / period;

    // ---- Display Current Frequency ----
    tft.fillRect(0, currentY, screenWidth, graphY - currentY, ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_YELLOW);
    String curStr = String(frequency, 1) + " Hz";
    int16_t x1, y1; uint16_t w, h;
    tft.getTextBounds(curStr, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((screenWidth - w)/2, currentY);
    tft.println(curStr);

    Serial.print("Freq: ");
    Serial.print(frequency, 1);
    Serial.println(" Hz");

    // ---- Plot frequency graph ----
    plotFrequencyGraph(frequency);

  } else {
    tft.fillRect(0, currentY, screenWidth, graphY - currentY, ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(10, currentY);
    tft.println("No Signal");
    Serial.println("IR ON, No signal detected");
  }
}

// ================= Graph Function =================
void plotFrequencyGraph(float freq) {
  // Map 0–30kHz → graph height
  float displayFreq = constrain(freq, 0, 30000);
  int yVal = graphY + graphHeight - map(displayFreq, 0, 30000, 0, graphHeight);

  // Erase old column
  tft.drawFastVLine(graphX, graphY + 1, graphHeight - 2, ST77XX_BLACK);
  // Draw new point
  tft.drawPixel(graphX, yVal, ST77XX_GREEN);

  graphX++;
  if (graphX >= screenWidth) {
    graphX = 0;
    tft.fillRect(1, graphY + 1, screenWidth - 2, graphHeight - 2, ST77XX_BLACK);
  }
}




