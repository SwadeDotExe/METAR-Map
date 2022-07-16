/*
     This code is taken from https://github.com/WKHarmon/led-sectional
     and re-written to provide RBGW support and integration with HomeKit.

*/

#include <ESP8266WiFi.h>
#include <FastLED.h>
#include "FastLED_RGBW.h"
#include <vector>
#include <PubSubClient.h>
using namespace std;
#define FASTLED_ESP8266_RAW_PIN_ORDER

// METAR Variables
#define NUM_AIRPORTS 14            // This is really the number of LEDs
#define WIND_THRESHOLD 25         // Maximum windspeed for green, otherwise the LED turns yellow
#define LOOP_INTERVAL 5000        // ms - interval between brightness updates and lightning strikes
#define DO_LIGHTNING true         // Lightning uses more power, but is cool.
#define DO_WINDS true             // color LEDs for high winds
#define REQUEST_INTERVAL 900000   // How often we update. In practice LOOP_INTERVAL is added. In ms (15 min is 900000)

// WiFi Credentials
const char ssid[] = "ssid";                  // your network SSID (name)
const char password[] = "password";                  // your network password (use for WPA, or use as key for WEP)
const char mqtt_server[] = "192.168.1.208";   // MQTT Server address

// LED Variables
#define DATA_PIN 14
int brightness = 100;
int oldBrightness = 100;

// List of Airports. Order here corresponds to order of LEDs
std::vector<String> airports({
  "KRSV",
  "KPRG",
  "KHUF",
  "KBMG",
  "KGPC",
  "KCFJ",
  "KTYQ",
  "KUMP",
  "KMQJ",
  "KEYE",
  "KIND",
  "KHFY",
  "KGEZ",
  "KBAK"
});

/* ----------------------------------------------------------------------- */

// Query Variables
#define READ_TIMEOUT 15 // Cancel query if no data received (seconds)
#define WIFI_TIMEOUT 60 // in seconds
#define RETRY_TIMEOUT 15000 // in ms
#define SERVER "www.aviationweather.gov"
#define BASE_URI "/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&hoursBeforeNow=3&mostRecentForEachStation=true&stationString="
boolean ledStatus = true; // used so leds only indicate connection status on first boot, or after failure
int loops = -1;
std::vector<unsigned short int> lightningLeds;
int status = WL_IDLE_STATUS;

// Initialize LEDs
CRGBW leds[NUM_AIRPORTS];       // Create LED channel array. Times by 4 because of RGBW
CRGB *ledsRGB = (CRGB *) &leds[0];

// Setup MQTT
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup() {

  // Initialize serial
  Serial.begin(115200);

  // Create LED array
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_AIRPORTS));

  // Test LEDS
  FastLED.setBrightness(brightness);     // Set brightness of LEDs to variable
  colorFill(CRGB::Red);
  colorFill(CRGB::Green);
  colorFill(CRGB::Blue);
  fillWhite();
  colorFill(CRGB::Black);


  // Setup WiFi
  setup_wifi();

  // Create MQTT Client
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // // Disable Onboard LED
  // pinMode(LED_BUILTIN, OUTPUT);
  // digitalWrite(LED_BUILTIN, LOW);
}

void loop() {

  int c;
  loops++;
  Serial.print("Loop: ");
  Serial.println(loops);
  unsigned int loopThreshold = 1;
  if (DO_LIGHTNING) loopThreshold = REQUEST_INTERVAL / LOOP_INTERVAL;

  // Connect to WiFi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  // Connect to MQTT if not conencted
  if (!client.connected()) {
    MQTTReconnect();
  }

  // Do some lightning
  if (DO_LIGHTNING && lightningLeds.size() > 0) {
    doLightning();
  }

  if (loops >= loopThreshold || loops == 0) {
    loops = 0;

    Serial.println("Getting METARs ...");
    if (getMetars()) {
      Serial.println("Refreshing LEDs.");
      FastLED.show();
      if ((DO_LIGHTNING && lightningLeds.size() > 0)) {
        Serial.println("There is lightning or we're using a light sensor, so no long sleep.");
        digitalWrite(LED_BUILTIN, HIGH);
        delayWithMQTT(LOOP_INTERVAL); // pause during the interval
      } else {
        Serial.print("No lightning; Going into sleep for: ");
        Serial.println(REQUEST_INTERVAL);
        digitalWrite(LED_BUILTIN, HIGH);
        delayWithMQTT(REQUEST_INTERVAL);
      }
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
      delayWithMQTT(RETRY_TIMEOUT); // try again if unsuccessful
    }
  }
  else { //
    digitalWrite(LED_BUILTIN, HIGH);
    delayWithMQTT(LOOP_INTERVAL); // pause during the interval
  }
}

bool getMetars(){
  lightningLeds.clear(); // clear out existing lightning LEDs since they're global
  colorFill(CRGB::Black); // Set everything to black just in case there is no report
  uint32_t t;
  char c;
  boolean readingAirport = false;
  boolean readingCondition = false;
  boolean readingWind = false;
  boolean readingGusts = false;
  boolean readingWxstring = false;

  std::vector<unsigned short int> led;
  String currentAirport = "";
  String currentCondition = "";
  String currentLine = "";
  String currentWind = "";
  String currentGusts = "";
  String currentWxstring = "";
  String airportString = "";
  bool firstAirport = true;
  for (int i = 0; i < NUM_AIRPORTS; i++) {
    if (airports[i] != "NULL" && airports[i] != "VFR" && airports[i] != "MVFR" && airports[i] != "WVFR" && airports[i] != "IFR" && airports[i] != "LIFR") {
      if (firstAirport) {
        firstAirport = false;
        airportString = airports[i];
      } else airportString = airportString + "," + airports[i];
    }
  }

  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (!client.connect(SERVER, 443)) {
    Serial.println("Connection failed!");
    client.stop();
    return false;
  } else {
    Serial.println("Connected ...");
    Serial.print("GET ");
    Serial.print(BASE_URI);
    Serial.print(airportString);
    Serial.println(" HTTP/1.1");
    Serial.print("Host: ");
    Serial.println(SERVER);
    Serial.println("User-Agent: LED Map Client");
    Serial.println("Connection: close");
    Serial.println();
    // Make a HTTP request, and print it to console:
    client.print("GET ");
    client.print(BASE_URI);
    client.print(airportString);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(SERVER);
    client.println("User-Agent: LED Sectional Client");
    client.println("Connection: close");
    client.println();
    client.flush();
    t = millis(); // start time
    FastLED.clear();

    Serial.print("Getting data");

    while (!client.connected()) {
      if ((millis() - t) >= (READ_TIMEOUT * 1000)) {
        Serial.println("---Timeout---");
        client.stop();
        return false;
      }
      Serial.print(".");
      delayWithMQTT(1000);
    }

    Serial.println();

    while (client.connected()) {
      if ((c = client.read()) >= 0) {
        yield(); // Otherwise the WiFi stack can crash
        currentLine += c;
        if (c == '\n') currentLine = "";
        if (currentLine.endsWith("<station_id>")) { // start paying attention
          if (!led.empty()) { // we assume we are recording results at each change in airport
            for (vector<unsigned short int>::iterator it = led.begin(); it != led.end(); ++it) {
              doColor(currentAirport, *it, currentWind.toInt(), currentGusts.toInt(), currentCondition, currentWxstring);
            }
            led.clear();
          }
          currentAirport = ""; // Reset everything when the airport changes
          readingAirport = true;
          currentCondition = "";
          currentWind = "";
          currentGusts = "";
          currentWxstring = "";
        } else if (readingAirport) {
          if (!currentLine.endsWith("<")) {
            currentAirport += c;
          } else {
            readingAirport = false;
            for (unsigned short int i = 0; i < NUM_AIRPORTS; i++) {
              if (airports[i] == currentAirport) {
                led.push_back(i);
              }
            }
          }
        } else if (currentLine.endsWith("<wind_speed_kt>")) {
          readingWind = true;
        } else if (readingWind) {
          if (!currentLine.endsWith("<")) {
            currentWind += c;
          } else {
            readingWind = false;
          }
        } else if (currentLine.endsWith("<wind_gust_kt>")) {
          readingGusts = true;
        } else if (readingGusts) {
          if (!currentLine.endsWith("<")) {
            currentGusts += c;
          } else {
            readingGusts = false;
          }
        } else if (currentLine.endsWith("<flight_category>")) {
          readingCondition = true;
        } else if (readingCondition) {
          if (!currentLine.endsWith("<")) {
            currentCondition += c;
          } else {
            readingCondition = false;
          }
        } else if (currentLine.endsWith("<wx_string>")) {
          readingWxstring = true;
        } else if (readingWxstring) {
          if (!currentLine.endsWith("<")) {
            currentWxstring += c;
          } else {
            readingWxstring = false;
          }
        }
        t = millis(); // Reset timeout clock
      } else if ((millis() - t) >= (READ_TIMEOUT * 1000)) {
        Serial.println("---Timeout---");
        colorFill(CRGB::Cyan); // indicate status with LEDs
        FastLED.show();
        ledStatus = true;
        client.stop();
        return false;
      }
    }
  }
  // need to doColor this for the last airport
  for (vector<unsigned short int>::iterator it = led.begin(); it != led.end(); ++it) {
    doColor(currentAirport, *it, currentWind.toInt(), currentGusts.toInt(), currentCondition, currentWxstring);
  }
  led.clear();

  // Do the key LEDs now if they exist
  for (int i = 0; i < (NUM_AIRPORTS); i++) {
    // Use this opportunity to set colors for LEDs in our key then build the request string
    if (airports[i] == "VFR") leds[i] = CRGB::Green;
    else if (airports[i] == "WVFR") leds[i] = CRGB::Yellow;
    else if (airports[i] == "MVFR") leds[i] = CRGB::Blue;
    else if (airports[i] == "IFR") leds[i] = CRGB::Red;
    else if (airports[i] == "LIFR") leds[i] = CRGB::Magenta;
  }

  client.stop();
  return true;
}

void doColor(String identifier, unsigned short int led, int wind, int gusts, String condition, String wxstring) {
  CRGB color;
  Serial.print(identifier);
  Serial.print(": ");
  Serial.print(condition);
  Serial.print(" ");
  Serial.print(wind);
  Serial.print("G");
  Serial.print(gusts);
  Serial.print("kts LED ");
  Serial.print(led);
  Serial.print(" WX: ");
  Serial.println(wxstring);
  if (wxstring.indexOf("TS") != -1) {
    Serial.println("... found lightning!");
    lightningLeds.push_back(led);
  }
  if (condition == "LIFR" || identifier == "LIFR") color = CRGB::Magenta;
  else if (condition == "IFR") color = CRGB::Red;
  else if (condition == "MVFR") color = CRGB::Blue;
  else if (condition == "VFR") {
    if ((wind > WIND_THRESHOLD || gusts > WIND_THRESHOLD) && DO_WINDS) {
      color = CRGB::Yellow;
    } else {
      color = CRGB::Green;
    }
  } else color = CRGB::Black;

  leds[led] = color;
  FastLED.show();
}

void colorFill(CRGB c){
  for(int i = 0; i < NUM_AIRPORTS; i++){
    leds[i] = c;
    FastLED.show();
    delay(50);
  }
  delayWithMQTT(500);
}

void colorFillShort(CRGB c){
  for(int i = 0; i < NUM_AIRPORTS; i++){
    leds[i] = c;
    FastLED.show();
    delay(1);
  }
}

void fillWhite(){
  for(int i = 0; i < NUM_AIRPORTS; i++){
    leds[i] = CRGBW(0, 0, 0, 255);
    FastLED.show();
    delay(50);
  }
  delayWithMQTT(500);
}

void setup_wifi() {

  delay(10);
  colorFillShort(CRGB::Blue);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    colorFillShort(CRGB::Black);
    delay(250);
    Serial.print(".");
    colorFillShort(CRGB::Blue);
    delay(250);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  colorFillShort(CRGB::Green);
  delay(100);
  colorFillShort(CRGB::Black);
  delay(100);
  colorFillShort(CRGB::Green);
  delay(100);
  colorFillShort(CRGB::Black);
  delay(100);
  colorFillShort(CRGB::Green);
  delay(100);
  colorFillShort(CRGB::Black);
}

// Get MQTT Message (Interrupt)
void callback(char topic[], unsigned char payload[], unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  boolean breakOut = false;
  char buffer[length];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    buffer[i] = (char)payload[i];
  }
  Serial.println();

  // Convert buffer to String
  String message = String(buffer);

  // Brightness Handler
  if (strcmp(topic, "METARMap/Brightness") == 0) {

    brightness = message.toInt();
    Serial.print("Updating Brightness to ");
    Serial.println(brightness);
    oldBrightness = brightness;
    FastLED.setBrightness(brightness);     // Set brightness of LEDs to variable
    FastLED.show();
    breakOut = true;
    break;
  }

  // Right Couch Handler
  if (strcmp(topic, "METARMap/State") == 0 && !breakOut) {
    Serial.print("Updating state to ");
    Serial.println(message);
    if (message == "true") {
      brightness = oldBrightness;
      FastLED.setBrightness(brightness);     // Set brightness of LEDs to variable
      FastLED.show();
    } else {
      oldBrightness = brightness;
      brightness = 0;
      FastLED.setBrightness(brightness);     // Set brightness of LEDs to variable
      FastLED.show();
    }
  }
}

void MQTTReconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("METARMap/State");
      client.subscribe("METARMap/Brightness");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void doLightning() {
  std::vector<CRGBW> lightning(lightningLeds.size());

  // Store original color
  for (unsigned short int i = 0; i < lightningLeds.size(); ++i) {
    unsigned short int currentLed = lightningLeds[i];
    Serial.print("Lightning on LED: ");
    Serial.println(currentLed);
    lightning[i] = leds[currentLed]; // temporarily store original color
  }

  // Do the lightning
  for (unsigned short int i = 0; i < 3; ++i) {
    for (unsigned short int i = 0; i < lightningLeds.size(); ++i) {
      unsigned short int currentLed = lightningLeds[i];
      leds[currentLed] = CRGBW(0, 0, 0, 255); // set to white briefly
      FastLED.show();
      delay(25);
      leds[currentLed] = CRGBW(0, 0, 0, 0); // set to black briefly
      FastLED.show();
      delay(75);
      leds[currentLed] = CRGBW(0, 0, 0, 255); // set to white briefly
      FastLED.show();
      delay(100);
      leds[currentLed] = CRGBW(0, 0, 0, 0); // set to black briefly
      FastLED.show();
      delay(75);
    }
  }
  delay(25); // extra delay seems necessary with light sensor
  FastLED.show();
  delay(25);
  for (unsigned short int i = 0; i < lightningLeds.size(); ++i) {
    unsigned short int currentLed = lightningLeds[i];
    leds[currentLed] = CRGBW(0, 0, 0, 0); // set to black
    leds[currentLed] = lightning[i]; // restore original color
  }
  FastLED.show();
}

void delayWithMQTT(int delay) {

  // Snapshot current time
  unsigned long currentMillis = millis();

  // Wait until time reaches (snapshot + delay)
  while (currentMillis + delay > millis()) {

    // Update MQTT
    client.loop();
  }
}
