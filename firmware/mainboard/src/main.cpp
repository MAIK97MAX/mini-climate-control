/*
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define OLED_SDA 21 
#define OLED_SCL 22

// Objekte Display und DHT-Sensor definieren
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
DHT dht(4, DHT11);

// Initialisierung des Displays, DHT-Sensors und der Seriellen Verbindung
void setup() {  
    Serial.begin(9600);
    delay(1000);
    Serial.println("boot");

    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(100000);

    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED init failed");
        while (true) delay(1000);
    }
    Serial.println("OLED ok");

    oled.clearDisplay();
    oled.ssd1306_command(SSD1306_DISPLAYON);
    oled.ssd1306_command(SSD1306_SETCONTRAST);
    oled.ssd1306_command(0xFF);
    oled.fillScreen(SSD1306_WHITE);   // whole screen white
    oled.display();
    Serial.println("filled white");

    dht.begin();
}

//DHT11 Werte auslesen und auf dem Display ausgeben
void loop() {
    float t = dht.readTemperature();
    if (isnan(t)) {
        Serial.println("DHT read failed");
        delay(2000);
        return;
    }
    Serial.println(t);

    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print("Temp: ");
    oled.print(t, 1);
    oled.println(" C");
    oled.display();
    delay(2000);
}
*/

#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21); // clock, data

void setup() {
  u8g2.begin();
}

void loop() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 20, "Hello ESP32");
  u8g2.sendBuffer();
  delay(1000);
}


