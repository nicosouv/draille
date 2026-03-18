#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS  10
#define TFT_DC   9
#define TFT_RST  8

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

void setup() {
    Serial.begin(115200);
    Serial.println("Wokwi test starting...");

    tft.begin();
    tft.fillScreen(ILI9341_RED);
    Serial.println("Red screen done");
    delay(1000);

    tft.fillScreen(ILI9341_GREEN);
    Serial.println("Green screen done");
    delay(1000);

    tft.fillScreen(ILI9341_BLUE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(3);
    tft.setCursor(30, 100);
    tft.print("Wokwi OK!");
    Serial.println("Setup complete");
}

void loop() {
    Serial.println("loop");
    delay(2000);
}
