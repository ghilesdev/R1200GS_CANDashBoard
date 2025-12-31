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
  uint8_t speed;
  uint8_t gear;
} canData_t;
canData_t receivedData;

// Simulation flag
bool simulateData = false;

// ESP-NOW callback
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
  if (len == sizeof(canData_t)) {
    memcpy(&receivedData, incomingData, len);
    Serial.println(receivedData.speed);
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
uint8_t lastOilTemp = 0; //optimise : do not render if value has not changed
uint8_t lastSpeed = 0; //optimise : do not render if value has not changed
char speed[4];
// 'station-essence', 32x32px
const unsigned char gasIcon [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0xf1, 0x80, 0x38, 0x00, 0x3b, 0x80, 
	0x30, 0x00, 0x39, 0xc0, 0x3f, 0xff, 0xf8, 0xe0, 0x3c, 0x00, 0xf8, 0x70, 0x3c, 0x00, 0xf8, 0x38, 
	0x3c, 0x00, 0xf8, 0x7c, 0x3c, 0x00, 0xf8, 0xfc, 0x3c, 0x00, 0xf8, 0xce, 0x3c, 0x00, 0xf8, 0xfe, 
	0x3c, 0x00, 0xf8, 0xfe, 0x3f, 0xff, 0xf8, 0x3e, 0x37, 0xff, 0xb8, 0x0e, 0x30, 0x00, 0x38, 0x0e, 
	0x30, 0x00, 0x3c, 0x0e, 0x30, 0x00, 0x3f, 0x0e, 0x30, 0x00, 0x3f, 0x8e, 0x30, 0x00, 0x3b, 0x8e, 
	0x30, 0x00, 0x3b, 0x8e, 0x30, 0x00, 0x3b, 0x8e, 0x30, 0x00, 0x3b, 0x8e, 0x30, 0x00, 0x39, 0xdc, 
	0x30, 0x00, 0x39, 0xfc, 0x30, 0x00, 0x38, 0xf8, 0x30, 0x00, 0x38, 0x00, 0x30, 0x00, 0x38, 0x00, 
	0x30, 0x00, 0x38, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
};
const unsigned char oilTempIcon [] PROGMEM = {
	0x00, 0x07, 0xc0, 0x00, 0x00, 0x0f, 0xe0, 0x00, 0x00, 0x0f, 0xe0, 0x00, 0x00, 0x0e, 0xee, 0x00, 
	0x00, 0x0c, 0x6e, 0x00, 0x00, 0x0c, 0x60, 0x00, 0x00, 0x0c, 0x6e, 0x00, 0x00, 0x0c, 0x6e, 0x00, 
	0x00, 0x0c, 0x60, 0x00, 0x00, 0x0c, 0x6e, 0x00, 0x00, 0x0c, 0x6e, 0x00, 0x00, 0x0d, 0x60, 0x00, 
	0x00, 0x0d, 0x6e, 0x00, 0x00, 0x0d, 0x6e, 0x00, 0x00, 0x0d, 0x60, 0x00, 0x00, 0x0d, 0x6e, 0x00, 
	0x00, 0x0d, 0x6e, 0x00, 0x00, 0x0d, 0x60, 0x00, 0x00, 0x0d, 0x6e, 0x00, 0x00, 0x1d, 0x7e, 0x00, 
	0x00, 0x3d, 0x78, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 
	0x00, 0x7f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 
	0x00, 0x7f, 0xfc, 0x00, 0x00, 0x3f, 0xf8, 0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x07, 0xc0, 0x00
};
// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 144)
const int epd_bitmap_allArray_LEN = 1;
const unsigned char* epd_bitmap_allArray[1] = {
	gasIcon
};

void drawXBM(int16_t x, int16_t y, const unsigned char *bitmap, int16_t w, int16_t h, uint16_t color) {
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      if (pgm_read_byte(&bitmap[j * (w / 8) + i / 8]) & (1 << (7 - (i % 8)))) {
        tft.drawPixel(x + i, y + j, color);
      }
    }
  }
}

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
  tft.setCursor(10, 40);
  tft.println("RPM: --");



  // draw gauges
  drawXBM(446, 78, gasIcon, 32, 32, 0xf00f);
  tft.drawRect(446, 110, totalFuelBarWidth, totalFuelBarHeight+1, 0xffff); //fuel
  tft.drawRect(395, 264, 50, 50, 0x0fff); // gear indcator
  drawXBM(10, 278, oilTempIcon, 32, 32, 0xf00f);



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
  tft.fillRect(50, 40, 200, 20, 0x0000); // Clear RPM value area
  tft.setCursor(50, 40);
  tft.print(receivedData.rpm);
  
  if (receivedData.oilTemp != lastOilTemp){
    tft.fillRect(45, 278, 100, 32, 0x0000); // Clear oil temp area
    tft.setCursor(45, 278);
    tft.setTextSize(4);
    tft.print(receivedData.oilTemp);
    tft.print(" C");
    tft.setTextSize(2); //always reset to default text size
    lastOilTemp = receivedData.oilTemp;
  }
  if (receivedData.speed != lastSpeed){
    snprintf(speed, sizeof(speed), "%03d", receivedData.speed);
    tft.fillRect(100, 100, 150, 60, 0x0000); // Clear oil temp area
    tft.setCursor(100, 100);
    tft.setTextSize(8);
    tft.print(speed);
    tft.setTextSize(3); //always reset to default text size
    tft.print(" KM/h");
    tft.setTextSize(2); //always reset to default text size
    lastOilTemp = receivedData.oilTemp;
  }



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
