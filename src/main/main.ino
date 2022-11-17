#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include "DallasTemperature.h"
#include <Arduino_JSON.h>
#include "secrets.h"

//#define DEBUG

// Use #define in secrets.h for your specific settings
// e.g. #define S_SSID "Your WiFi SSID"

const char* ssid = S_SSID;
const char* pswd = S_PSWD;
const char* mqtt_server = S_MQTT_SERVER;
const char* mqttUser = S_MQTT_USER;
const char* mqttPass = S_MQTT_PASS;

const char* topic = "spa";
const char* clientId = "controller";

long timeBetweenMessages = 1000 * 20 * 1;

float hysteresis = 0.2;

//Pin config
#define ONE_WIRE_BUS D4
#define PUMP_PIN D2
#define HEATER_PIN D1

//System wide states
bool pumpStatus;
bool heaterStatus;
bool statusChange;
float tempFloat;

//int tempF;
//int tempLast;
int tempSP;
bool updateRecv = false;
int status = WL_IDLE_STATUS;     // the starting Wifi radio's status


//Client instantiations
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient espClient;
PubSubClient client(espClient);

float round_to_dp( float in_value, int decimal_place )
{
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier ) / multiplier;
  return in_value;
}

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
}
void setup_ota()
{
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    #ifdef DEBUG
    Serial.println("Start updating " + type);
    #endif
  });
  ArduinoOTA.onEnd([]() {
    #ifdef DEBUG
    Serial.println("\nEnd");
    #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef DEBUG
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif
  });
  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef DEBUG
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
    #endif
  });
  ArduinoOTA.begin();
}

void callback(char* topic, byte* payload, unsigned int length)
{
  //{"pump": 0/1, "tempSP": degF};
  //Debug print code
  #ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  #endif
  JSONVar inMessage = JSON.parse((char *) payload);
  //Serial.println(JSON.typeof(inMessage));
  //Serial.println(inMessage);
  if(inMessage.hasOwnProperty("pump"))//Make sure that "pump" is part of the message
  {
    pumpStatus = (bool) inMessage["pump"];
	updateRecv = true;
	#ifdef DEBUG
    Serial.print("Pump set to: ");
    Serial.println(pumpStatus);
	#endif
  }
  // if(inMessage.hasOwnProperty("temperature"))
  // {
  //   tempSP = (int) inMessage["temperature"];
	// updateRecv = true;
	// #ifdef DEBUG
  //   Serial.print("Temp set to: ");
  //   Serial.println(tempSP);
	// #endif
  // }
  if(inMessage.hasOwnProperty("tempSP"))
  {
    tempSP = (int) inMessage["tempSP"];
    updateRecv = true;
    #ifdef DEBUG
    Serial.print("Temp set to:");
    Serial.println(tempSP)
    #endif
  }
}
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
	#ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
	#endif

    // Attempt to connect
    if (client.connect(clientId,mqttUser,mqttPass))
    {
	  
	  client.publish("homeassistant/spa", ("connected"), true);
	  client.subscribe("homeassistant/spa/set");
	  #ifdef DEBUG
      Serial.println("connected");
	  #endif
	  
    } else {
	  #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
	  #endif
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);

  tempSP = 40;
  pumpStatus = false;
  heaterStatus = false;
  statusChange = false;
  
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
  setup_wifi();
  setup_ota();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //Initialize the temp sensor
  sensors.begin();
}

JSONVar outMessage;
long now, lastMsg;
void loop()
{
  //Verify connection at the top of loop
  if (!client.connected())
  {
    reconnect();//Reconnect if not
  }
  client.loop();//Allow the MQTT API to run it's backend shit
  ArduinoOTA.handle();//Listen  for OTA updates

  //check the temp
  // Need to wait for temp to be stable before trusting the temp reading
  sensors.requestTemperatures();
  tempFloat = sensors.getTempFByIndex(0);
  //tempF = (int) tempFloat;
  
  digitalWrite(PUMP_PIN, pumpStatus);//May have to change polarity depending on SSR
  outMessage["pump"] = pumpStatus;
  outMessage["heater"] = false;
  if(pumpStatus)
  {
    if(tempFloat<tempSP-hysteresis)
    {
      digitalWrite(HEATER_PIN, true);
      outMessage["heater"] = true;
      if(heaterStatus == false)
        statusChange = true;
      else
        statusChange = false;
      heaterStatus = true;
    }
    else if(tempFloat>tempSP+hysteresis)
    {
      digitalWrite(HEATER_PIN, false);
      outMessage["heater"] = false;
      if(heaterStatus == true)
        statusChange = true;
      else
        statusChange = false;
      heaterStatus = false;
    }
  }
  else
  {
    digitalWrite(HEATER_PIN, false);
    outMessage["heater"] = false;
    if(heaterStatus == true)
      statusChange = true;
    else
      statusChange = false;
    heaterStatus = false;
  }
  outMessage["temperature"] = tempFloat;
  outMessage["tempSP"] = tempSP;

  //Serialize and broadcast on a timer or if a command was received

  now = millis();
  if( ((now - lastMsg) > timeBetweenMessages) || updateRecv || statusChange)
  {
    lastMsg = now;
    #ifdef DEBUG
    Serial.print("Publish message: ");
    Serial.println(outMessage);
	  #endif
	
	  client.publish("homeassistant/spa/state", JSON.stringify(outMessage).c_str());
	  updateRecv = false;
  }
}
