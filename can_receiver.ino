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
typedef struct {
  uint16_t rpm;
  uint8_t fuelLevel;
  uint8_t oilTemp;
} canData_t;
canData_t receivedData;

// Simulation flag
bool simulateData = false;

// ESP-NOW callback
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
  if (len == sizeof(receivedData)) {
    memcpy(&receivedData, incomingData, len);
  }
}
// for drawing
uint8_t totalFuelBarHeight = 200; //optimise : do not render if value has not changed
uint8_t totalFuelBarWidth = 30; //optimise : do not render if value has not changed
uint8_t lastFuel = 0; //optimise : do not render if value has not changed

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
  tft.drawRect(446, 110, totalFuelBarWidth, totalFuelBarHeight, 0xffff);

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
  uint8_t fuel_bar_height = (uint8_t) map(receivedData.fuelLevel, 0, 100, 0, totalFuelBarHeight-2);
  if (lastFuel != fuel_bar_height){

    tft.fillRect(447, 309-1-lastFuel, totalFuelBarWidth-2, lastFuel, 0x0000); // first erase level 
    tft.fillRect(447, 309-1-fuel_bar_height, totalFuelBarWidth-2, fuel_bar_height, 0xffff); // then redraw whole recatngle, is it better to draw and erase segments?? 
    lastFuel = fuel_bar_height;
  }

  delay(50); // ~10 FPS refresh rate
}
