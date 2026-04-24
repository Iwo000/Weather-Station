/*Original code by Iwo Arkadiusz Malecki

Uses Xiao ESP32 C3, https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/ 

https://github.com/Iwo000/Weather-Station

Commenting out serial after testing to reduce battery usage                                      
*/

#include <DHT.h>
#include <HTTPClient.h>
#include <WiFi.h>

// Setup Sensors
const int battPin = A0; // Pin for mesuring battery procentage 
const int soilMoistPin = A1;  // Soil moisture sensor
const int lightPin = A2; // Pin of photoresistor
const int dhtPin = D3; // DHT 11
const int dhtType = DHT11;
// Naming DHt; declaring pin and type
DHT dht(dhtPin, dhtType);

// WiFi credentals

//const char* ssid = "yourSSID";

//const char* password = "yourPassword";

// More WiFi stuff
const char* receiverIP = "http://192.168.0.148/data";

// Deep Sleep length, in seconds
const int sleepySeconds = 15 * 60;

void setup() {
  Serial.begin(115200);

  dht.begin();  // Starts up DHT sensor
  pinMode(soilMoistPin, INPUT);
  pinMode(lightPin, INPUT);
  pinMode(battPin, INPUT);
  // Set ADC resolution, Needed for soil moisture sensor
  analogReadResolution(12);  // 0–1024
  Serial.print("\n\n Booting up");

  // Delay so components fully activate before reading
  delay(2000);

  // DHT read values, Humidity and Temperature
  float humidVal = dht.readHumidity();    // Read dht air mositure
  float tempVal = dht.readTemperature();  // Read dht air temperature

    // Check if DHT data recevived is valid, if not go to sleep and attempt next awakening
  if (isnan(tempVal) || isnan(humidVal)) {
    Serial.println("Failed to read from DHT11, going to sleep");
    //sleepyTime();  // Sleep, try again next cycle
  }

  // Analog Data
  //Analog read at 3.3V can be incosistent, so reading 10 times then dividing gives more accurate data
  float moistValCorrection = 0;
  for (int i = 1; i<= 10; i++) {
    moistValCorrection = moistValCorrection + analogReadMilliVolts(soilMoistPin);
  }
  moistValCorrection = moistValCorrection / 10;
  float moistVal = (3300.0 - moistValCorrection) / 3300.0 * 100.0; // Soil moisture sensor value
  // 3300mV means low, 0mV means high
  
  float lightValCorrection = 0;
  for (int i = 1; i <= 10; i++) {
    lightValCorrection = lightValCorrection + analogReadMilliVolts(lightPin);
  }
  lightValCorrection = lightValCorrection / 10;
  float lightVal = (3300.0 - lightValCorrection) / 3300.0 * 100.0; // photoresistor data
  // 3300mV means low, 0mV means high 

  uint32_t voltBatt = 0; // Battery reading from official Xiao Wiki
  for(int i = 0; i < 16; i++) {
    voltBatt = voltBatt + analogReadMilliVolts(battPin); // ADC with correction   
  }
  float battVal = 2 * voltBatt / 16 / 1000.0;     // attenuation ratio 1/2, mV --> V


  // Print values in serial
  Serial.printf("\nHumid:\t %.1f%%", humidVal);
  Serial.printf("\nTemp:\t %.1fC", tempVal);
  Serial.printf("\nSoil Moisture:\t %.1f%%", moistVal);
  Serial.printf("\nLight:\t %.1f%%", lightVal);
  Serial.printf("\nBattery:\t %.2fV", battVal);

  // Setup WiFi
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");

  // Try connecting to WiFi
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries <= 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  // If WiFi still has not connected, try next cycle
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Couldnt connect, restaring");
    sleepyTime();
  }

  Serial.println("\nConnected to WiFi");
  delay(500);

  // Create a message that will be sent to reciver
  String message = "tempVal=" + String(tempVal) + "&humidVal=" + String(humidVal) + "&moistVal=" + String(moistVal) + "&lightVal=" + String(lightVal) + "&battVal=" + String(battVal);

  HTTPClient http; // HTTP client object for making web requests
  http.begin(receiverIP); // Set target server, trough IP

  http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Describing data format to reciver

  int responseCode = http.POST(message); // Send message, and recive HTTP response code

  // Print response code, 200 = good, 4xx = client error, 5xx = server error, -1 = no connection
  Serial.printf("\nServer response: %d", responseCode);

  http.end(); // Close HTTP request
  WiFi.disconnect(true); // Make sure WiFi disconnects to avoid future problems

  sleepyTime(); // After all is done, go to sleep
}

void loop() {
  // Void loop is empty
}

// Defining sleepyTime Command
void sleepyTime() {
  Serial.printf("\nSleeping for %d seconds", sleepySeconds);
  Serial.flush(); // Waits til all serial has been printed befor continuing
  esp_sleep_enable_timer_wakeup((uint64_t)sleepySeconds * 1000000ULL); // Setting wakeup timer to SLEEP_
  esp_deep_sleep_start();
}