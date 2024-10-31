#include <DHT.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

// Wi-Fi and ThingSpeak Settings
const char* ssid = "YOUR_WIFI_SSD"; //Replace with your Wifi SSD
const char* password = "YOUR_WIFI_PASSWORD";  //Replace with your Wifi Password 
const char* server = "api.thingspeak.com";
String apiKey = "YOUR_API_KEY";  //Replace with your API KEY

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // No reset pin

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT Sensor Settings
#define DHTPIN D4  // GPIO4 on NodeMCU
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Gas Sensor
#define sensor A0

int gasLevel = 0;
String quality = "";
unsigned long previousMillis = 0;
const long interval = 5000;  // Switch display every 5 seconds
bool showAirQuality = true;  // Toggle display state

// Wi-Fi Setup
void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("IP Address: " + WiFi.localIP().toString());
}

// Air Quality Calculation and Display
void air_sensor() {
  gasLevel = analogRead(sensor);

  if (gasLevel < 60) {
    quality = "GOOD!";
  } else if (gasLevel <= 90) {
    quality = "Poor!";
  } else if (gasLevel <= 150) {
    quality = "Very Bad!";
  } else if (gasLevel <= 500) {
    quality = "Toxic!";
  } else {
    quality = "Hazardous!";
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 5);
  display.println("Air Quality:");
  display.setCursor(0, 25);
  display.print("Level: ");
  display.println(gasLevel);
  display.setCursor(0, 45);
  display.println("Status: " + quality);
  display.display();
}

// DHT Sensor Data Display
void sendSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor! Check wiring.");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("DHT Read Error!");
    display.display();
    return;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("DHT SENSOR DATA");
  display.setCursor(0, 20);
  display.print("Temp: ");
  display.print(t);
  display.println(" C");

  display.setCursor(0, 35);
  display.print("Humidity: ");
  display.print(h);
  display.println(" %");

  display.display();

  // Send data to ThingSpeak
  sendToThingSpeak(t, h, gasLevel);
}

// Send Data to ThingSpeak
void sendToThingSpeak(float temperature, float humidity, int gas) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    String url = "/update?api_key=" + apiKey +
                 "&field1=" + String(temperature) +
                 "&field2=" + String(humidity) +
                 "&field3=" + String(gas);

    if (client.connect(server, 80)) {
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + server + "\r\n" +
                   "Connection: close\r\n\r\n");
      client.stop();
      Serial.println("Data sent to ThingSpeak");
    } else {
      Serial.println("Connection to ThingSpeak failed.");
    }
  } else {
    Serial.println("WiFi disconnected!");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(sensor, INPUT);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1);  // Halt if OLED fails
  }
  display.clearDisplay();

  // Initialize DHT sensor with a delay
  dht.begin();
  delay(2000);  // Give the sensor time to stabilize

  // Connect to Wi-Fi
  connectWiFi();

  // Initial Display Message
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(10, 0);
  display.println("Air Monitor");
  display.setTextSize(1);
  display.setCursor(15, 20);
  display.println("By Circuit Digest");
  display.display();
  delay(2000);
  display.clearDisplay();
}

void loop() {
  unsigned long currentMillis = millis();

  // Switch between Air Quality and DHT Data every 5 seconds
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    showAirQuality = !showAirQuality;  // Toggle display
  }

  if (showAirQuality) {
    air_sensor();  // Display air quality data
  } else {
    sendSensor();  // Display DHT11 sensor data
  }

  delay(500);  // Small delay for stability
}
