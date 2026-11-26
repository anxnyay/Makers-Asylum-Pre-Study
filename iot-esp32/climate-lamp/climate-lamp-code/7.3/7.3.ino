#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

/************************* Pin Definitions *********************************/
#define PIN_NEOPIXEL 13
#define NUM_PIXELS   16
#define PIN_DHT      15    
#define DHTTYPE      DHT11

/************************* Object Initialization ***************************/
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
DHT dht(PIN_DHT, DHTTYPE);

/************************* WiFi & Adafruit IO Setup *****************************/
#define WLAN_SSID       "Ananya"
#define WLAN_PASS       "."
#define AIO_USERNAME    "anxnyay"
#define AIO_KEY         "."

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, "io.adafruit.com", 1883, AIO_USERNAME, AIO_KEY);

// --- Publishers (Outgoing Sensor Data to Adafruit IO Dashboard) ---
Adafruit_MQTT_Publish temp_publish = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish hum_publish = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");

/************************* Global Variables *********************************/
unsigned long lastSensorUpdate = 0;

void MQTT_connect();
void updateMoodlamp(float t, float h);

void setup() {
  Serial.begin(115200);
  
  // Hardware Setup
  strip.begin();
  strip.setBrightness(200);
  strip.show(); // Clear ring (off)
  dht.begin();

  // WiFi Connection
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void loop() {
  MQTT_connect();

  // Read sensors and update LEDs + Adafruit IO every 5 seconds
  if (millis() - lastSensorUpdate > 5000) {
    lastSensorUpdate = millis();
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      Serial.print("Temp: "); Serial.print(t); Serial.print(" °C | ");
      Serial.print("Humidity: "); Serial.print(h); Serial.println(" %");

      // 1. Update Moodlamp LEDs based on conditions
      updateMoodlamp(t, h);

      // 2. Publish values to Adafruit IO Dashboard
      temp_publish.publish(t);
      delay(1000); // Staggered delay for MQTT broker stability
      hum_publish.publish(h);
      
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
  }

  if (!mqtt.ping()) mqtt.disconnect();
}

/************************* LED Logic *********************************/

void updateMoodlamp(float t, float h) {
  uint32_t tempColor;
  uint32_t humColor;

  // Temperature Logic: Left 8 LEDs (Indices 0 to 7)
  if (t < 32.0) {
    tempColor = strip.Color(0, 0, 255);   // BLUE
  } else {
    tempColor = strip.Color(255, 0, 0);   // RED
  }

  // Humidity Logic: Right 8 LEDs (Indices 8 to 15)
  if (h < 50.0) {
    humColor = strip.Color(255, 255, 0);  // YELLOW
  } else {
    humColor = strip.Color(0, 0, 255);    // BLUE
  }

  // Set Left Half (0 - 7) for Temperature
  for (int i = 0; i < 8; i++) {
    strip.setPixelColor(i, tempColor);
  }

  // Set Right Half (8 - 15) for Humidity
  for (int i = 8; i < 16; i++) {
    strip.setPixelColor(i, humColor);
  }

  strip.show(); // Apply colors to physical NeoPixels
}

void MQTT_connect() {
  if (mqtt.connected()) return;
  Serial.print("Connecting to MQTT... ");
  while (mqtt.connect() != 0) {
    mqtt.disconnect();
    delay(5000);
  }
  Serial.println("MQTT Connected!");
}
