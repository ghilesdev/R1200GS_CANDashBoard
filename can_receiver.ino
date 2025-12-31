#include <SPI.h>
#include <ILI9486_SPI.h>
#include <esp_now.h>
#include <WiFi.h>

// Display Pins
#define TFT_CS   10
#define TFT_DC   4
#define TFT_RST  5
#define TFT_MOSI 7
#define TFT_MISO -1
#define TFT_SCLK 6
ILI9486_SPI tft(TFT_CS, TFT_DC, TFT_RST);

// Data structure for ESP-NOW
typedef struct __attribute__((packed)){
  uint16_t rpm;
  uint8_t fuelLevel;
  uint8_t oilTemp;
  uint8_t gear;
} canData_t;
canData_t receivedData;

// Simulation flag
bool simulateData = false;

// ESP-NOW callback
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
  if (len == sizeof(canData_t)) {
    memcpy(&receivedData, incomingData, len);
  } else {
    Serial.println("impossible to copy received data");
    Serial.println(len);
    Serial.println(sizeof(canData_t));

  }
}
// for drawing
uint8_t totalFuelBarHeight = 200; //optimise : do not render if value has not changed
uint8_t totalFuelBarWidth = 30; //optimise : do not render if value has not changed
uint8_t lastFuelSegmentsNb = 0; //optimise : do not render if value has not changed
uint8_t lastGear = 0; //optimise : do not render if value has not changed

void setup() {
  Serial.begin(115200);
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setTextSize(2);

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  Serial.println(WiFi.macAddress());
  esp_now_register_recv_cb(OnDataRecv);

  // Print initial labels
  tft.setCursor(10, 10);
  tft.println("RPM: --");
  tft.setCursor(10, 40);
  tft.println("Oil Temp: -- C");
  tft.setCursor(10, 70);
  tft.println("Fuel: -- %");

  // draw gauges
  tft.drawRect(446, 110, totalFuelBarWidth, totalFuelBarHeight+1, 0xffff); //fuel
  tft.drawRect(395, 264, 50, 50, 0x0fff); // gear indcator

}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "test on") {
      simulateData = true;
      Serial.println("Simulation mode ON");
    } else if (cmd == "test off") {
      simulateData = false;
      Serial.println("Simulation mode OFF");
    }
  }

  // Simulate or use real data
  if (simulateData) {
    receivedData.rpm = random(0, 10000);
    receivedData.oilTemp = random(60, 120);
    receivedData.fuelLevel = random(0, 100);
    // receivedData.gear = random(0, 6);
  }

  // Update display
  tft.fillRect(50, 10, 200, 20, 0x0000); // Clear RPM value area
  tft.setCursor(50, 10);
  tft.print(receivedData.rpm);

  tft.fillRect(110, 40, 100, 20, 0x0000); // Clear oil temp area
  tft.setCursor(110, 40);
  tft.print(receivedData.oilTemp);
  tft.print(" C");

  tft.fillRect(70, 70, 100, 20, 0x0000); // Clear fuel level area
  tft.setCursor(70, 70);
  tft.print(receivedData.fuelLevel);
  tft.print(" %");

  // fuel level:
  uint8_t segmentsNb = (uint8_t) map(receivedData.fuelLevel, 0, 100, 0, 20);
  if (lastFuelSegmentsNb != segmentsNb){
    if (segmentsNb > lastFuelSegmentsNb){ // draw only added segments
      tft.fillRect(447, 310-(10*segmentsNb), totalFuelBarWidth-2, (segmentsNb-lastFuelSegmentsNb)*10, 0xffff);
    } else if (segmentsNb < lastFuelSegmentsNb) { // less fuel, erase corresponding segemnts
      tft.fillRect(447, 310-(10*lastFuelSegmentsNb), totalFuelBarWidth-2, (lastFuelSegmentsNb-segmentsNb)*10, 0x0000);
    }
    lastFuelSegmentsNb = segmentsNb;
  }

  // Gear indicator
  if (receivedData.gear != lastGear) {
    tft.fillRect(396, 265, 48, 48, 0x0000); // Clear gear indicator area
    tft.setCursor(406, 275);
    tft.setTextSize(5);
    if (receivedData.gear == 0){
      tft.setTextColor(0x0f0f);
      tft.print("N");
    } else {
      tft.setTextColor(0xffff);
      tft.print(receivedData.gear);
    }
    tft.setTextSize(2);
    receivedData.gear = lastGear;
  }


  delay(50); // ~10 FPS refresh rate
}
