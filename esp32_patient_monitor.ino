#include <WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Wire.h>
#include <MAX30100.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Define your Wi-Fi credentials and ThingSpeak settings
const char* ssid = "Sri";
const char* password = "dhanu002";
unsigned long channelID = 2786321;
const char* writeAPIKey = "7OBCFISEX2YAWYHA";

// DHT11 sensor setup
#define DHTPIN 4            // Pin connected to DHT11
#define DHTTYPE DHT11      
DHT dht(DHTPIN, DHTTYPE);

// MAX30100 setup
MAX30100 pox;  // Declare the MAX30100 object
float heartRate, SpO2;

// Brevo (Sendinblue) API settings for email/SMS
const String brevoAPI = "https://api.brevo.com/v3/smtp/email"; 
const String apiKey = "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";

// Time intervals
unsigned long lastTime = 0;
unsigned long interval = 10000; // 10 seconds for data upload

// Thresholds for triggering alerts
float temperatureThreshold = 37.5; 
float heartRateLowThreshold = 60;  
float heartRateHighThreshold = 100;
float SpO2Threshold = 90;

void setup() {
Serial.begin(115200);
dht.begin();// Initialize DHT11
Wire.begin();// Initialize MAX30100
pox.begin();  // Start the MAX30100 sensor

// Connect to Wi-Fi
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
delay(1000);
Serial.println("Connecting to WiFi...");
}
Serial.println("Connected to WiFi");

// Initialize ThingSpeak with WiFiClient
WiFiClient client;
ThingSpeak.begin(client);
}

void loop() {
// Read sensor data
float temperature = dht.readTemperature();
pox.update();  // Update the MAX30100 sensor data
heartRate = pox.getHeartRate();
SpO2 = pox.getSpO2();

// Check if the readings are valid
if (isnan(temperature) || isnan(heartRate) || isnan(SpO2)) {
Serial.println("Failed to read from sensors!");
return;
}

// Display sensor readings
Serial.print("Temperature: ");
Serial.print(temperature);
Serial.print(" °C, Heart Rate: ");
Serial.print(heartRate);
Serial.print(" bpm, SpO2: ");
Serial.println(SpO2);

// Upload to ThingSpeak every interval
if (millis() - lastTime > interval) {
lastTime = millis();

ThingSpeak.setField(1, temperature);
ThingSpeak.setField(2, heartRate);
ThingSpeak.setField(3, SpO2);

int httpCode = ThingSpeak.writeFields(channelID, writeAPIKey);
if (httpCode == 200) {
Serial.println("Data uploaded to ThingSpeak");
} else {
Serial.println("Failed to upload to ThingSpeak");
}
}


// Trigger Brevo notification if SpO2, heart rate, or temperature is abnormal
if (SpO2 < SpO2Threshold || heartRate < heartRateLowThreshold || heartRate > heartRateHighThreshold || temperature > temperatureThreshold) {
sendBrevoAlert(temperature, heartRate, SpO2);
}

delay(2000); // Delay for a while before the next reading
}
// Function to send email alert via Brevo API
void sendBrevoAlert(float temperature, float heartRate, float SpO2) {
HTTPClient http;

// Prepare JSON data for the email/SMS
String jsonData = "{";
jsonData += "\"sender\": {\"email\": \"vsbdhanu20@gmail.com\"},";
jsonData += "\"to\": [{\"email\": \"sriiiiii2021@gmail.com\"}],";
jsonData += "\"htmlContent\": \"<html><body><h2>Warning!</h2><p>";
jsonData += "Temperature: " + String(temperature) + "°C<br>";
jsonData += "Heart Rate: " + String(heartRate) + " bpm<br>";
jsonData += "SpO2: " + String(SpO2);
jsonData += "}";

// Send HTTP request to Brevo API
http.begin(brevoAPI);
http.addHeader("Content-Type", "application/json");
http.addHeader("api-key", apiKey); // Set Brevo API key
int httpResponseCode = http.POST(jsonData);
if (httpResponseCode > 0) {
Serial.println("Alert sent via Brevo!");
} else {
Serial.println("Failed to send alert via Brevo");
}
http.end();
}

