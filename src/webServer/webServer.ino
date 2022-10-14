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
enum opMode
{
  AUTO,
  ON,
  OFF
};

enum opMode state;
bool pumpStatus;
bool heaterStatus;
int tempMeas;
int tempLast;
int tempSP;

void spaControlLoop()
{
  //check the temp
  sensors.requestTemperatures();
  tempLast = tempMeas;
  tempMeas= sensors.getTempFByIndex(0);

  switch (state)
  {
  case AUTO:
    #ifdef DEBUG
    Serial.println("Mode set to AUTO");
    #endif
    //Check the time
    //turn on pump if applicable
      //turn on heater if needed
    break;
  case ON:
    #ifdef DEBUG
    Serial.println("Mode set to ON");
    #endif
    //turn on pump
    digitalWrite(PUMP_PIN, true);
    //turn on heater if needed
    if(tempMeas<tempSP)
    {
      #ifdef DEBUG
      Serial.println("Heater ON");
      #endif
      digitalWrite(HEATER_PIN, true);//May have to change polarity depending on SSR
    }
    else
    {
      #ifdef DEBUG
      Serial.println("Heater OFF");
      #endif
      digitalWrite(HEATER_PIN, false);//May have to change polarity depending on SSR
    }
    break;
  case OFF:
    #ifdef DEBUG
    Serial.println("Mode set to OFF");
    #endif
    //turn off pump
    digitalWrite(PUMP_PIN, false);
    //turn off heater
    digitalWrite(HEATER_PIN, false);
    break;
  default:
    break;
  }

}

//Webserver functions
void handleRoot()
{
  server.send(200, "text/html", "<!DOCTYPE html>\
<html>\
<head>\
<title>Hot Tub Interface</title>\
<meta name=\"viewport\"content=\"width=device-width,initial-scale=1\">\
<style>\
html {\
font-family:Arial;\
display:inline-block;\
margin:0px auto;\
text-align:center;\
}\
h1 {\
color:#0F3376;\
padding:2vh;\
font-variant:small-caps;\
}\
p {\
font-size: 1.5rem;\
}\
.btn {\
display:inline-block;\
background-color:#df8918;\
border:none;\
border-radius:4px;\
color:white;\
padding:16px 40px;\
text-decoration:none;\
font-size:30px;\
margin:2px;\
cursor:pointer;\
}\
.btnA {\
background-color:#1cd4f5;\
}\
.btnO{\
background-color: #d61e06;\
}\
.btnI{\
background-color: #078832;\
}\
.btnD{\
background-color:#99500b;\
}\
.units {\
font-size:1.2rem;\
vertical-align:middle\
}\
.sensor-labels {\
font-size:1.5rem;\
vertical-align:middle;\
padding-bottom:15px;\
}\
</style>\
</head>\
<body>\
<h1>Hot Tub Controller</h1>\
<p>\
<a href=\"/on\"><button class=\"btn\">ON</button></a>\
<a href=\"/auto\"><button class=\"btn btnA\">AUTO</button></a>\
<a href=\"/off\"><button class=\"btn btnO\">OFF</button></a>\
</p>\
<p>\
<a href=\"/inc\"><button class=\"btn btnI\">+</button></a>\
<a href=\"/dec\"><button class=\"btn btnD\">-</button></a>\
</p>\
<p>\
<span class=\"sensor-labels\">Current Temp: </span>\
<span class=\"sensor-labels\" id=\"tempMeas\">%TEMPMEAS%</span>\
<sup class=\"units\">°F</sup>\
</p>\
<p>\
<span class=\"sensor-labels\">Temp Setpoint: </span>\
<span class=\"sensor-labels\" id = \"tempSP\">%TEMPSP%</span>\
<sup class=\"units\">°F</sup>\
</p>\
</body>\
<script>\
function request(elementId)\
{\
var xhttp=new XMLHttpRequest();\
xhttp.onreadystatechange=function(){\
if (this.readyState==4&&this.status==200){\
document.getElementById(elementId).innerHTML=this.responseText;\
}\
};\
xhttp.open(\"GET\",\"/\"+elementId,true);\
xhttp.send();\
};\
window.onload = function() {\
request(\"tempMeas\");\
request(\"tempSP\");\
};\
setInterval(request(\"tempMeas\"),250);\
setInterval(request(\"tempSP\"),250);\
</script>\
</html>"\
);
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Go fuck yourself");
}

void redirectToRoot()
{
  server.sendHeader("Location","/"); //Tell the webpage to go back to the root
  server.send(303); //Redirect
}

void handleOn()
{
  redirectToRoot();
  state = ON;
}

void handleAuto()
{
  redirectToRoot();
  state = AUTO;
}

void handleOff()
{
  redirectToRoot();
  state = OFF;
}

void handleTempMeas()
{
  char buff[5];
  itoa(tempMeas,buff,10);
  server.send(200, "text/plain", buff);
}

void handleTempSP()
{
  char buff[5];
  itoa(tempSP,buff,10);
  server.send(200,"text/plain",buff);
}

void handleInc()
{
  redirectToRoot();
  ++tempSP;
}

void handleDec()
{
  redirectToRoot();
  --tempSP;
}

void http_init()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/on", HTTP_GET, handleOn);
  server.on("/auto", HTTP_GET, handleAuto);
  server.on("/off", HTTP_GET, handleOff);
  server.on("/inc", HTTP_GET, handleInc);
  server.on("/dec", HTTP_GET, handleDec);
  server.on("/tempMeas", HTTP_GET, handleTempMeas);
  server.on("/tempSP", HTTP_GET, handleTempSP);
  server.onNotFound(handleNotFound);
  server.begin();
  #ifdef DEBUG
  Serial.println("HTTP server started");
  #endif
}

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  #ifdef DEBUG
  Serial.begin(115200); 
  #endif  
  setup_wifi();
  http_init();

  //Initialize the temp sensor
  sensors.begin();
}

void loop()
{
  spaControlLoop();
  server.handleClient();
}
