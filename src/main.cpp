/**
 * ----------------------------------------------------------------------------
 * ESP32 Remote Control with WebSocket
 * ----------------------------------------------------------------------------
 * © 2020 Stéphane Calderoni
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------

#define LED_PIN   26
#define LED_BUILTIN 33
#define BTN_PIN   22
#define HTTP_PORT 80

#define NUM_LEDS 8
#define DATA_PIN 12

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// Button debouncing
const uint8_t DEBOUNCE_DELAY = 10; // in milliseconds

// WiFi credentials
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASS = "YOUR_WIFI_PASSWORD";

const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

void notifyClients(void);

// ----------------------------------------------------------------------------
// Definition of the LED component
// ----------------------------------------------------------------------------

struct Led {
    // state variables
    uint8_t pin;
    bool    on;

    // methods
    void update() {
        digitalWrite(pin, on ? HIGH : LOW);
    }
};

// ----------------------------------------------------------------------------
// Definition of the Button component
// ----------------------------------------------------------------------------

struct Button {
    // state variables
    uint8_t  pin;
    bool     lastReading;
    uint32_t lastDebounceTime;
    uint16_t state;

    // methods determining the logical state of the button
    bool pressed()                { return state == 1; }
    bool released()               { return state == 0xffff; }
    bool held(uint16_t count = 0) { return state > 1 + count && state < 0xffff; }

    // method for reading the physical state of the button
    void read() {
        // reads the voltage on the pin connected to the button
        bool reading = digitalRead(pin);

        // if the logic level has changed since the last reading,
        // we reset the timer which counts down the necessary time
        // beyond which we can consider that the bouncing effect
        // has passed.
        if (reading != lastReading) {
            lastDebounceTime = millis();
        }

        // from the moment we're out of the bouncing phase
        // the actual status of the button can be determined
        if (millis() - lastDebounceTime > DEBOUNCE_DELAY) {
            // don't forget that the read pin is pulled-up
            bool pressed = reading == LOW;
            if (pressed) {
                     if (state  < 0xfffe) state++;
                else if (state == 0xfffe) state = 2;
            } else if (state) {
                state = state == 0xffff ? 0 : 0xffff;
            }
        }

        // finally, each new reading is saved
        lastReading = reading;
    }
};

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

Led    onboard_led = { LED_BUILTIN, false };
Led    led         = { LED_PIN, false };
Button button      = { BTN_PIN, HIGH, 0, 0 };

uint8_t ledstate=0;

CRGB leds[NUM_LEDS];

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");
AsyncUDP udp;

// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Cannot mount SPIFFS volume...");
    while (1) {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
    }
  }
}

// ----------------------------------------------------------------------------
// UDP handler
// ----------------------------------------------------------------------------
void onUdpPacket(AsyncUDPPacket packet)
{
   uint32_t plen=packet.length();
   uint8_t *pdata;
   uint8_t x;

   if (plen==2)
   {
       pdata=packet.data();
       if (pdata[0]==74)
       {
        if (pdata[1] == 1) {
            if (ledstate!=1)
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0xFF0000; 
                ledstate=1;
            }
            else
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0; 
                ledstate=0;            
            }
        }
        if (pdata[1] == 3) {
            if (ledstate!=2)
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x00FF00; 
                ledstate=2;
            }
            else
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0; 
                ledstate=0;            
            }
        }
        if (pdata[1] == 4) {
            if (ledstate!=3)
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0000FF; 
                ledstate=3;
            }
            else
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0; 
                ledstate=0;            
            }
        }
        if (pdata[1] == 2) {
            if (ledstate!=4)
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x808000; 
                ledstate=4;
            }
            else
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0; 
                ledstate=0;            
            }
        }

        FastLED.show(); 
        notifyClients();

       }
   }
}


// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi() {
  /*WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());*/
   WiFi.disconnect();   //added to start with the wifi off, avoid crashing
   WiFi.mode(WIFI_OFF); //added to start with the wifi off, avoid crashing
   WiFi.mode(WIFI_AP);
   WiFi.softAP(ssid, password);
   /*while (WiFi.getMode() != WIFI_AP) 
   {
        delay(50);
   }
  IPAddress IP(4,3,2,1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(IP, IP, NMask);*/

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP); 

  udp.listen(1234);
  udp.onPacket(onUdpPacket);

  uint8_t x;
  for (x=0; x<NUM_LEDS; x++) 
  {
    leds[x] = CRGB::Green; 
    if (x>0) leds[x-1] = 0x0; 
    FastLED.show();
    delay(250);
  }
  leds[NUM_LEDS-1] = 0x0; 
  FastLED.show();

}




// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var) {
    //return String(var == "STATE" && led.on ? "on" : "off");
	return String(var == "STATE" && led.on ? "off" : "off");
}

void onRootRequest(AsyncWebServerRequest *request) {
     request->send(SPIFFS, "/index.html", "text/html", false, processor);
}

void initWebServer() {
    server.on("/", onRootRequest);
    server.serveStatic("/", SPIFFS, "/");
    server.begin();
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

void notifyClients() {
    const uint8_t size = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<size> json;
    if (ledstate==0) json["status"] = "off";
    if (ledstate==1) json["status"] = "red";
    if (ledstate==2) json["status"] = "green";
    if (ledstate==3) json["status"] = "blue";
    if (ledstate==4) json["status"] = "yellow";
    //json["status"] = ledstate ? "on" : "off";

    char buffer[32];
    size_t len = serializeJson(json, buffer);
    ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

        const uint8_t size = JSON_OBJECT_SIZE(1);
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }

        const char *action = json["action"];
        uint8_t x;
        Serial.println(action);

        if (strcmp(action, "red") == 0) {
            if (ledstate!=1)
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0xFF0000; 
                ledstate=1;
            }
            else
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0; 
                ledstate=0;            
            }
        }
        if (strcmp(action, "green") == 0) {
            if (ledstate!=2)
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x00FF00; 
                ledstate=2;
            }
            else
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0; 
                ledstate=0;            
            }
        }
        if (strcmp(action, "blue") == 0) {
            if (ledstate!=3)
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0000FF; 
                ledstate=3;
            }
            else
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0; 
                ledstate=0;            
            }
        }
        if (strcmp(action, "yellow") == 0) {
            if (ledstate!=4)
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x808000; 
                ledstate=4;
            }
            else
            {
                for (x=0; x<NUM_LEDS; x++) leds[x] = 0x0; 
                ledstate=0;            
            }
        }

        FastLED.show(); 
        notifyClients();

    }
}

void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len) {

    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup() {
    pinMode(onboard_led.pin, OUTPUT);
    pinMode(led.pin,         OUTPUT);
    pinMode(button.pin,      INPUT);

    Serial.begin(115200); delay(500);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

    initSPIFFS();
    initWiFi();
    initWebSocket();
    initWebServer();
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop() {
    ws.cleanupClients();

    button.read();

    if (button.pressed()) {
        led.on = !led.on;
        notifyClients();
    }
    
    onboard_led.on = millis() % 1000 < 50;

    led.update();
    onboard_led.update();
}