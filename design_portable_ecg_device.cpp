#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

// Hardware pins
#define ECG_PIN        A0      // AD8232 OUT → A0
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1      // Reset pin not used

Adafruit_SH1106 display(OLED_RESET);

// Plotting state
int xPos       = 0;
int prevY      = SCREEN_HEIGHT / 2;
const int WAVE_HEIGHT = 48;  // Reserve bottom 16px for BPM

// BPM (Big‑Box) variables
const int   R_THRESHOLD = 550;    // Tune this to the signal’s R‑peak amplitude
unsigned long lastRTime = 0;
float        bpm       = 0;

void setup() {
  Serial.begin(9600);

  // Initialize OLED via I2C (address 0x3C)
  if (!display.begin(SH1106_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED not found"));
    while (true);
  }

  display.clearDisplay();
  // Splash screen
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Portable ECG Monitor");
  display.display();
  delay(2000);
  display.clearDisplay();
}

void loop() {
  // 1) Read raw ECG and map to top region
  int raw = analogRead(ECG_PIN);
  int y   = map(raw, 0, 1023, WAVE_HEIGHT - 1, 0);

  // 2) Draw ECG segment in the top 48px
  display.drawLine(xPos, prevY, xPos + 1, y, WHITE);
  prevY = y;

  // 3) Advance/Wrap X and clear entire waveform area if needed
  if (++xPos >= SCREEN_WIDTH) {
    xPos = 0;
    // Clear only waveform region (0–WAVE_HEIGHT)
    display.fillRect(0, 0, SCREEN_WIDTH, WAVE_HEIGHT, BLACK);
  }

  // 4) R‑peak detection + BPM calc (Big‑Box method)
  unsigned long now = millis();
  if (raw > R_THRESHOLD && now - lastRTime > 300) {
    if (lastRTime) {
      float dt_s = (now - lastRTime) / 1000.0;
      bpm = 60.0 / dt_s;
    }
    lastRTime = now;
  }

  // 5) Clear bottom region (to erase old BPM) and print new BPM
  display.fillRect(0, WAVE_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - WAVE_HEIGHT, BLACK);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, WAVE_HEIGHT + 2);   // A couple px of top padding
  if (bpm > 0) {
    display.print((int)bpm);
  } else {
    display.print("--");
  }
  display.print(" BPM");

  // 6) Push all graphics to the screen
  display.display();

  delay(10);
}
