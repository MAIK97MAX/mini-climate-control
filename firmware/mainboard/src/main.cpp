#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>

#define OLED_SDA 21
#define OLED_SCL 22
#define DHT_PIN  4
#define LED_PIN 2

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
DHT dht(DHT_PIN, DHT11);

float lastTemp = NAN;
float lastHum  = NAN;
String mode = "off";

WebServer server(80);
unsigned long lastRead = 0;

unsigned long lastBlink = 0;
bool ledState = false;

// Raindrop icon, about 9 px wide and 13 px tall, top-left at (x, y)
void drawDrop(int x, int y) {
  u8g2.drawTriangle(x + 4, y, x, y + 7, x + 8, y + 7);
  u8g2.drawDisc(x + 4, y + 8, 4);
}

// Draw the entire screen, including temperature, humidity and mode
void drawScreen() {
  u8g2.clearBuffer();

  // The temperature is drawn in the center of the screen, with a large font
  const int baseline = 44;
  char tempStr[8];
  if (isnan(lastTemp)) snprintf(tempStr, sizeof(tempStr), "--");
  else                 snprintf(tempStr, sizeof(tempStr), "%.1f", lastTemp);

  u8g2.setFont(u8g2_font_logisoso32_tr);
  int tw = u8g2.getStrWidth(tempStr);
  int cw = u8g2.getStrWidth("C");
  int total = tw + 3 + 8 + cw;          // number + gap + degree sign + C
  int x = (128 - total) / 2;

  u8g2.drawStr(x, baseline, tempStr);

  // Degree sign is drawn as two concentric circles, with a radius of 3 and 2 pixels, respectively
  // values are hardcoded to position the degree sign correctly relative to the temperature value
  int dx = x + tw + 3 + 3;
  int dy = baseline - 32 + 4;
  u8g2.drawCircle(dx, dy, 3);
  u8g2.drawCircle(dx, dy, 2);

  u8g2.drawStr(x + tw + 3 + 8, baseline, "C");

  // The Humidity is drawn in the bottom-right corner, with a small drop icon next to it
  // it calls the drawDrop() function to draw the drop icon, and then draws the humidity value next to it
  char humStr[8];
  if (isnan(lastHum)) snprintf(humStr, sizeof(humStr), "--%%");
  else                snprintf(humStr, sizeof(humStr), "%.0f%%", lastHum);

  u8g2.setFont(u8g2_font_helvB08_tr);
  int hw = u8g2.getStrWidth(humStr);
  int hx = 127 - hw;
  u8g2.drawStr(hx, 63, humStr);
  drawDrop(hx - 13, 51); // values are hardcoded to position the drop icon correctly

  // Mode is drawn in the bottom-left corner, with a label depending on the mode
  // lastly all the buffer is sent to the display
  const char* label = "OFF";
  if (mode == "cooling") label = "COOLING";
  else if (mode == "heating") label = "HEATING";

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 63, label);

  u8g2.sendBuffer();
}

// This function is called when the client requests the webpage
// It serves the index.html file from LittleFS. If the file is not found, it returns a 404 error.
void handleRoot() {
  File f = LittleFS.open("/index.html", "r");
  if (!f) {
    server.send(404, "text/plain", "index.html not found");
    return;
  }
  server.streamFile(f, "text/html");
  f.close();
}

// This function is called when the client requests /data
// It returns A .json object with the current temperature, humidity and mode
void handleData() {
  char json[96];
  char t[12], h[12];

  if (isnan(lastTemp)) strcpy(t, "null");
  else                 snprintf(t, sizeof(t), "%.1f", lastTemp);

  if (isnan(lastHum))  strcpy(h, "null");
  else                 snprintf(h, sizeof(h), "%.0f", lastHum);  
  
  snprintf(json, sizeof(json), "{\"temp\":%s,\"hum\":%s,\"mode\":\"%s\"}", t, h, mode.c_str());  
  server.send(200, "application/json", json);
}

// This function is called when the client requests /mode?value=...
// It sets the mode to "off", "cooling" or "heating" and
// returns a simple "ok" response. If the value is invalid, it returns a 400 error.
void handleMode() {

  // Simple debug output to the serial monitor, showing the number of arguments and their names and values
  Serial.printf("args: %d\n", server.args());
  for (int i = 0; i < server.args(); i++) {
    Serial.printf("  %s = %s\n", server.argName(i).c_str(), server.arg(i).c_str());
  }

  // If no value is provided, return a 400 error with a message
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "missing value");
    return;
  }

  String v = server.arg("value");

  // Check if the value is valid and set the mode accordingly
  // If the value is valid, redraw the screen and send a 200 response. If not, send a 400 response.
  if (v == "off" || v == "cooling" || v == "heating") {
    mode = v;
    drawScreen();
    server.send(200, "text/plain", "ok");
  } else {
    server.send(400, "text/plain", "bad value");
  }
}

void setup() {
  Serial.begin(9600);
  delay(1000);                 
  dht.begin();
  u8g2.begin();
  drawScreen();

  /*    Initializing LittleFS     */

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  } else {
    Serial.println("LittleFS ready");
  }

  // Here every file in the LittleFS root directory is listed, along with its size in bytes.
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  Serial.println("Files in LittleFS:");
  while (file) {
    Serial.printf(" %s (%u bytes)\n", file.name(), (unsigned)file.size());
    file = root.openNextFile();
  }
  Serial.println("--- end of list ---");

  /*    Initializing WiFI in AP Mode and mDNS     */

  WiFi.mode(WIFI_AP);
  WiFi.softAP("SmartClimate", "smartclimate123");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  if (MDNS.begin("smartclimate")) {
    Serial.println("mDNS ready: smartclimate.local");
  }

  /*    Initializing Server Functions     */

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/mode", handleMode);
  server.serveStatic("/", LittleFS, "/"); // must come last otherwise it will override the other handlers
  server.begin();

  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  digitalWrite(LED_PIN, LOW);
}

// Main action loop, It handles the client requests 
// and reads the DHT sensor every 2 seconds
// If the reading is successful, it updates the lastTemp and lastHum variables
// and redraws the screen. If the reading fails, it keeps the last good values and prints an error message in the serial monitor.
void loop() {
  server.handleClient();

  if (millis() - lastRead >= 2000) {
    lastRead = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("DHT read failed");   // keep showing the last good values
    } else {
      lastTemp = t;
      lastHum  = h;
      Serial.printf("Temp: %.1f C  Hum: %.0f %%\n", t, h);
    }

    drawScreen();

  }                
}                 





