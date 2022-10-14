#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include "DallasTemperature.h"
#include <Arduino_JSON.h>
#include "secrets.h"

#define DEBUG

// Use #define in secrets.h for your specific settings
// e.g. #define S_SSID "Your WiFi SSID"

const char* ssid = S_SSID;
const char* pswd = S_PSWD;
const char* mqtt_server = S_MQTT_SERVER;
const char* mqttUser = S_MQTT_USER;
const char* mqttPass = S_MQTT_PASS;

long timeBetweenMessages = 1000 * 20 * 1;

//Web server stuff
const char* domainName = "hottub";
ESP8266WebServer server(80);

//Pin config
#define ONE_WIRE_BUS 2
#define PUMP_PIN 0
#define HEATER_PIN 4

//System wide states
bool pumpStatus;
bool heaterStatus;
int tempF;
int tempLast;
int tempSP;
int status = WL_IDLE_STATUS;     // the starting Wifi radio's status


//Client instantiations
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  WiFi.begin(ssid, pswd);
  #ifdef DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  #endif

  if(MDNS.begin(domainName))
  {
    #ifdef DEBUG
    Serial.println("mDNS responder started");
  }
  else
  {
    Serial.println("Error setting up MDNS responder");
    #endif
  }
}

void spaControlLoop()
{
  //check the temp
  sensors.requestTemperatures();
  tempLast = tempF;
  tempF = sensors.getTempFByIndex(0);
  
  digitalWrite(PUMP_PIN, pumpStatus);//May have to change polarity depending on SSR

  if(pumpStatus)
  {
    if(tempF<tempSP)
    {
      digitalWrite(HEATER_PIN, true);//May have to change polarity depending on SSR
    }
    else
    {
      digitalWrite(HEATER_PIN, false);//May have to change polarity depending on SSR
    }
  }

}

//Webserver functions
void handleRoot()
{
  server.send(200, "text/plain", "Hello world!");
}

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  #ifdef DEBUG
  Serial.begin(115200); 
  #endif  
  setup_wifi();
  //Initialize the temp sensor
  sensors.begin();
}

void loop()
{

}
