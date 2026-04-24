/*Original code by Iwo Arkadiusz Malecki

https://github.com/Iwo000/Weather-Station
                                          
*/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <rgb_lcd.h> // The library still works for non rgb seeed studio lcd, but will not display colors
#include <time.h>
#include <OneWire.h>
#include <DallasTemperature.h>


//const char* ssid = "yourSSID";

//const char* password = "yourPassword";

// Port of webserver, port 80 is standard
AsyncWebServer server(80);

// Variables for Web server
float TempOut = 1; 
float tempIn = 1;
float HumidOut = 1;
float soilMoist = 1;
float lightOut = 1;
float gasIn = 1;
float ppm = 1;
float battV =1;

// Other variables
char timeStr[9]; //Define timeStr with max 9 characters, 8 characters plus 1 null terminator
char dataTime[9]; //
int lcdSlide = 1; // Slide of the LCD, from 1 to 3
const int sensorDelay = 5000; // Delay between inside temp readings in milliseconds
int lastDelay = 1; // last delay, will be replaced with millis after temp reading

// Pins used
const int SDApin = 8; // Since the esp32S3 im using does not have specified pin for SDA and SCL they need to be defined, most gpio pins can be used.
const int SCLpin = 9; // Feel free to remove this, and a later segment if your esp32 has specific I2C pins.
const int scrollPin = 6; // Pin of scroll button
const int gasPin = 4; // pin for gas detector
const int ONE_WIRE_BUS = 5; // Data wire is plugged into pin 2 on the Arduino, temp sensor

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// For Displaing time
const char* ntpServer = "pool.ntp.org"; // URL for ntp server, used for getting time
// Offset from GMT in seconds
const long gmtOffset = 3600; //Norway is GMT +1. 1 hour = 3600 seconds
// How much daylight savings offsets time.
const long DaylightOffset = 3600; //  During daylight savings, time = gmtOffset + DaylightOffset.

rgb_lcd lcd; // Define object lcd as an lcd. This works for non rgb lcd.

const char webpage[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
    <head>
        <title>
             Weather station
        </title>
        <meta name = "viewpoint" content="width=device-width, initial-scale=1">
        <meta http-equiv="refresh" content="30">
        <style>

            body, html {
            text-align: center;
            background-color: #111111;
            color: #f0f0f0;
            } 

            .container {
                display: flex;
                flex-wrap: wrap;
                justify-content: center;
                gap: 10px;
                padding: 20px;
            }
            .box {
                background: #282828;
                border-radius: 15px; 
                padding: 20px 20px;
                min-width: 200px;
                
            }

            h1{
               font: bold 30px/1.5 Helvetica;
            }

            p { 
                font: oblique 16px/1.5 Arial, sans-serif; 
            }

            .label { 
                font: bold 42px/1.5 helvetica;
                color: #f0f0f0;
            }

            .label-water { 
                font: bold 42px/1.5 helvetica;
                color: #6769ff;
            }

            .label-temp { 
                font: bold 42px/1.5 helvetica;
                color: #d35b5b;
            }

            .label-air { 
                font: bold 42px/1.5 helvetica;
                color: #dddddd;
            }

            .value { 
                font: oblique 16px/1.5 Arial, sans-serif; 
            }

            meter { 
                width: 50%; 
                margin-top: 12px; 
                height: 5px;
            }
        </style>
        
    </head>
    <body>

        <h1>
            Weather station Home
        </h1>

        <div class="label">
            Temperature And Humidity
        </div>

        <div class="container">

            <div class="box">
            <div class="label-temp"> Temperature Inside</div>
            <div class="value"> tempIn-placeholder &deg;C </div>
            <br>
            <meter value="temp-placeholder" min="-40" max="60"> </meter>
            <br>
            </div>
        
            <div class="box">
            <div class="label-temp"> Temperature Outside</div>
            <div class="value"> tempOut-placeholder &deg;C </div>
            <br>
            <meter value="temp-placeholder" min="-40" max="60"> </meter>
            <br>
            </div>
       
            <div class="box">
            <div class="label-water"> Humidity Outside </div>
            <div class="value"> humid-placeholder % </div>
            <br>
            <meter value="humid-placeholder" min="0" max="100"> </meter>
            <br>
            </div>
        

            <div class="box">
            <div class="label-water"> Ground Moisture </div>
            <div class="value"> moist-placeholder % </div>
            <br>
            <meter value="moist-placeholder" min="0" max="100"> </meter>
            <br>
            </div>

            <div class="box">
            <div class="label-air"> Light </div>
            <div class="value"> light-placeholder % </div>
            <br>
            <meter value="light-placeholder" min="0" max="100"> </meter>
            <br>
            </div>

            <div class="box">
            <div class="label-air"> Gas level </div>
            <div class="value"> gas-placeholder % </div>
            <br>
            <meter value="gas-placeholder" min="0" max="100"> </meter>
            <br>
            </div>
        </div>
        
        <footer>
            <p>
                Last update at last-update
                <br>
                Battery Voltage at battery-voltage ( 4.2V full, 3.6V - 3.9V ok, 3.0V - 3.4V needs charging)
                <br>
                Made by: Iwo Arkadiusz Malecki
            </p>
            <br>
        </footer>

    </body>
</html>
)rawhtml";

void setup() {
    // Setup code here and Web Server code
    Serial.begin(115200);
    lcd.begin(16, 2); // lcd.begin(Width, Height); Dimensions of the lcd,
    sensors.begin();
    pinMode(scrollPin, INPUT);
    // The esp32 S3 Dev kit im using does not have specified SDA or SCL pins. Therefore we must define them.
    Wire.begin(SDApin, SCLpin); // Wire.begin(SDA, SCL); Note if you esp has them specified pins feel free to delete this command

    delay(1000);

    // Starting up WiFi
    WiFi.begin(ssid, password);
    Serial.println("connecting");

    // Attempting to connecting to WiFi
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries <= 20) {
        delay(500);
        Serial.print(".");
        tries++;
    }

    // If WiFi still has not connected, restart
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Couldnt connect, restaring");
        ESP.restart();
    }
    
    Serial.println("\nConnected");// Connected to WiFi
    delay(500);
    Serial.println(WiFi.localIP()); // Print local ip
  
    // Setup and get time
    configTime(gmtOffset, DaylightOffset, ntpServer);
   
    // Turn on server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {

    String page = webpage;

    // Replace displayed data with new recived data
    page.replace("tempOut-placeholder", String(TempOut, 1));
    page.replace("tempIn-placeholder", String(tempIn, 1));
    page.replace("humid-placeholder", String(HumidOut, 1));
    page.replace("moist-placeholder", String(soilMoist, 1));
    page.replace("light-placeholder", String(lightOut, 1));
    page.replace("gas-placeholder", String(gasIn, 1));
    page.replace("battery-voltage", String(battV));
    page.replace("last-update", String(dataTime));

    // Send replaced html code to client with HTTP 200 (OK)
    request->send(200, "text/html", page);
  });
    
    // Turn on server directory for reciving data, /data
    server.on("/data", HTTP_POST, [](AsyncWebServerRequest* request) {
        // If valid data has been recived&&
        if (request->hasParam("tempVal", true) && request->hasParam("humidVal", true) && request->hasParam("moistVal", true) && request->hasParam("lightVal", true) && request->hasParam("battVal", true)) {
            Serial.println("\nReciving values");

            // Get data and convert to float
            TempOut = request->getParam("tempVal", true)->value().toFloat();
            HumidOut = request->getParam("humidVal", true)->value().toFloat();
            soilMoist = request->getParam("moistVal", true)->value().toFloat();
            lightOut = request->getParam("lightVal", true)->value().toFloat();
            battV = request->getParam("battVal", true)->value().toFloat();

            printLocalTime();
            strcpy(dataTime, timeStr); // Define at what time the data is recived
            // Print values in serial
            Serial.printf("\nTemp:\t %.1fC", TempOut);
            Serial.printf("\nHumid:\t %.1f%%", HumidOut);
            Serial.printf("\nSoil moisture:\t %.1f%%", soilMoist);
            Serial.printf("\nLight:\t %.1f%%", lightOut);
    }  
    request->send(200, "text/plain", "OK");
  });

  server.begin(); // Turn on web server
  Serial.println("Server ready");
}

void loop() {
    // Code for lcd and inside sensor, to run repeatedly:
    
    // If WiFi disconnects, retry connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost, reconnecting");
        WiFi.reconnect();
     
        delay(500);
    }

    // Inside Sensor data reading
    if (millis() - lastDelay >= sensorDelay) {
        sensors.requestTemperatures();
        tempIn = sensors.getTempCByIndex(0);
        ppm = map(analogRead(gasPin), 0, 4095, 0, 1000);
        lastDelay = millis();
        Serial.println("\nGetting inside sensors");
        Serial.printf("\nTemp In:\t %.1fC", tempIn);
        Serial.printf("\nPPM:\t %.1f", ppm);
    }

    // Change lcdSlide
    if (digitalRead(scrollPin) == HIGH) {
        delay(500);
        Serial.print("\nChanging slide");
        lcd.clear();
        // If current slide is slide 0 or 1, set slide to next slide
        if (lcdSlide == 1 || lcdSlide == 2) {
            lcdSlide ++;
            Serial.printf("\nSlide is now %1d%", lcdSlide);
        }
        // If current slide is slide 2, set slide back to 0
        else if (lcdSlide == 3) {
            lcdSlide = 1;
            Serial.printf("\nSlide is now %1d%", lcdSlide);
        }
    }

    // Display Chosen lcd slide
    if (lcdSlide == 1) { // Slide 1 is for Temperature
        lcd.setCursor(0,0);
        lcd.printf("Temp Out %.1fC", TempOut); // Temperature outside
        lcd.setCursor(0,1);
        lcd.printf("Temp In %.1fC", tempIn); // Temperature inside
        delay(100);
    }
    
    else if (lcdSlide == 2) { // Slide to is for 'water related' data
        lcd.setCursor(0,0);
        lcd.printf("Humidity %.1f%%", HumidOut); // Humidity outside
        lcd.setCursor(0,1);
        lcd.printf("Soil Moist %.1f%%", soilMoist); // misture in soil
        delay(100);
    }

    else if (lcdSlide == 3) { // Slide for 'air related' data
        lcd.setCursor(0,0);
        lcd.printf("Light %.1f%%", lightOut); // Light leve
        lcd.setCursor(0,1);
        lcd.printf("PPM: %.1f", ppm); // Gas ppm, particles per minute
        delay(100);
    }
}

void printLocalTime(){
  struct tm localTime;
  if(!getLocalTime(&localTime)) {
    strcpy(timeStr, "00:00:00");
    return;
  }
  strftime(timeStr, sizeof(timeStr), "%T", &localTime); // Turn &localTime into string timeStr with time format as "%T",HH:MM:SS (Hour, Minute, Second)
}
