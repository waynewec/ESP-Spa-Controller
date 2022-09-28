#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include "DallasTemperature.h"
#include <Arduino_JSON.h>
#include "secrets.h"

#define DEBUG

// Update these with values suitable for your network.

const char* ssid = S_SSID;
const char* pswd = S_PSWD;
const char* mqtt_server = S_MQTT_SERVER;
const char* mqttUser = S_MQTT_USER;
const char* mqttPass = S_MQTT_PASS;

const char* topic = "spa";//"wemos";    // this is the [root topic]
const char* clientId = "controller";

long timeBetweenMessages = 1000 * 20 * 1;

//Pin config
#define ONE_WIRE_BUS 2
#define PUMP_PIN 0
#define HEATER_PIN 4

//System wide states
bool pumpStatus;
bool heaterStatus;
int tempF;
int tempSP;
int status = WL_IDLE_STATUS;     // the starting Wifi radio's status


//Client instantiations
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient espClient;
PubSubClient client(espClient);

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
    //Serial.print("Pump set to: ");
    //Serial.println(pumpStatus);
  }
  if(inMessage.hasOwnProperty("tempSP"))
  {
    tempSP = (int) inMessage["tempSP"];
    //Serial.print("Temp set to: ");
    //Serial.println(tempSP);
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
	  
	  const char* configTopic = "homeassistant/sensor/spaTemp/config";
	  char *configTopics = 
	  {
		"homeassistant/sensor/spaTemp/config",
		"homeassistant/switch/spaPump/config",
		"homeassistant/sensor/spaHeater/config"
		"homeassistant/
	  }
	  
	  
	  
	  
	  
	  
	  
	  
	  
	  
	  
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topic, ("connected") , true );
      // ... and resubscribe
      String subscription = String(topic);
      subscription += "/";
      subscription += clientId;
      subscription += "/in";
      client.subscribe(subscription.c_str());
      //Serial.print("subscribed to : ");
      //Serial.println(subscription);
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

publishDiscovery()
{
  
}



void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //Initialize the temp sensor
  sensors.begin();

  publishDiscovery();
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

  //check the temp
  sensors.requestTemperatures();
  tempF = sensors.getTempFByIndex(0);
  
  digitalWrite(PUMP_PIN, pumpStatus);//May have to change polarity depending on SSR
  outMessage["pump"] = pumpStatus;
  outMessage["heater"] = false;
  if(pumpStatus)
  {
    if(tempF<tempSP)
    {
      digitalWrite(HEATER_PIN, true);//May have to change polarity depending on SSR
      outMessage["heater"] = true;
    }
    else
    {
      digitalWrite(HEATER_PIN, false);
      outMessage["heater"] = false;
    }
  }
  outMessage["tempAct"] = tempF;

  //Serialize and broadcast on a timer

  now = millis();
  if(now - lastMsg > timeBetweenMessages)
  {
    lastMsg = now;
    String pubTopic;
    pubTopic += topic ;
    pubTopic += "/";
    pubTopic += clientId;
    pubTopic += "/out";
    #ifdef
    //Serial.print("Publish topic: ");
    //Serial.println(pubTopic);
    //Serial.print("Publish message: ");
    //Serial.println(outMessage);
    client.publish((char *) pubTopic.c_str(), JSON.stringify(outMessage).c_str());
  }
}
