/*********
 * ESPGate by Raomin - June 2021
 * 
 * Control your electric gate from home assistant.
 *  
*********/

#include "config.h"

#ifdef ESP32
#include <WiFi.h>
#include <mDNS.h>
#elif ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

char devicename[32];

bool action;
bool stdby;
unsigned long first=0;

bool gateClosed;


void doAction(){
    digitalWrite(outputPin, action);
    


    if (gateClosed)
    client.publish("home/gated/state", "opening");
    else
    client.publish("home/gated/state", "closing");
    client.publish("home/gate/state", "ON");

    delay(500);
    digitalWrite(outputPin, stdby);
    client.publish("home/gate/state", "OFF");

}

void callback(char *topic, byte *payload, unsigned int length)
{
  char message[length + 1];
  strncpy(message, (char *)payload, length);
  message[length] = '\0';

  client.publish("home/gate/log", message);

  // check if payload is "OPEN" or "CLOSE" or "STOP"
  if (strcmp(message, "OPEN") == 0)
  {
    // action = HIGH;
    // stdby = LOW;
    doAction();
  }
  else if (strcmp(message, "CLOSE") == 0)
  {
    // action = LOW;
    // stdby = HIGH;
    doAction();
  }
  else if (strcmp(message, "STOP") == 0)
  {
    doAction();
  }
  else if (strcmp(message, "ON") == 0)
  {
    doAction();
  }
  else if (strcmp(message, "OFF") == 0)
  {
    doAction();
  }

  
}

void setup()
{
  Serial.begin(115200);

  sprintf(devicename,"espGate-%d",ESP.getChipId());
  ESP.getChipId();

  // Initialize the output variables as outputs
  if (pullup)
  {
    action = LOW;
    stdby = HIGH;
  }
  else
  {
    action = HIGH;
    stdby = LOW;
  }
  pinMode(D0, INPUT);
  gateClosed = false;
  pinMode(outputPin, OUTPUT);
  digitalWrite(outputPin, stdby);


  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ArduinoOTA.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

#ifdef ESP32
  mdns_hostname_set("espgate");
#elif ESP8266
  if (!MDNS.begin(devicename))
  {
    Serial.println("Error starting mDNS");
  }
#endif
}

void reconnect()
{
  // Loop until we're reconnected
  int tries = 0;
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(devicename, "", "", "home/gate/LWT", 0, true, "offline"))
    {
      // Once connected, publish an announcement...

      client.publish("homeassistant/cover/gated/config", "{\"name\": \"ESP Gated\",\"~\":\"home/gated\",\"uniq_id\":\"gated\",\"dev_cla\":\"door\",\"avty_t\":\"~/LWT\",\"command_topic\": \"~/set\", \"stat_t\": \"~/state\"}", true);
      client.publish("homeassistant/switch/gate/config", "{\"name\": \"ESP Gate\",\"~\":\"home/gate\",\"uniq_id\":\"gate\",\"avty_t\":\"~/LWT\",\"command_topic\": \"~/set\", \"stat_t\": \"~/state\"}", true);
      client.publish("home/gate/LWT", "online", true);
      client.publish("home/gated/LWT", "online", true);
      client.subscribe("home/gate/set");
      client.subscribe("home/gated/set");
      Serial.println("Connected!");
      bool status = digitalRead(D0)==HIGH;
      client.publish("home/gate/log", status ? "HIGH" : "LOW");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      unsigned long start = millis();
      while (millis() < (start + 5000L))
      {
        ArduinoOTA.handle();
      }
      if (tries++ == 10)
      {
#ifdef ESP32
        esp_restart();
#elif ESP8266
        Serial.println("10 failures, restart");
        ESP.reset();
#endif
      }
    }
  }
}


bool hasGateChanged(bool update=false)//return true if gate state has changed
{
  bool status = digitalRead(D0)==HIGH;
  if (status != gateClosed)
  {
    if (update)
      gateClosed = status;
    return true;
  }
  return false;
}

uint16 last = 0;

void loop()
{

  if (hasGateChanged())
  {
    //debounce
    delay(500);
    hasGateChanged(true);
    client.publish("home/gated/state", gateClosed ? "closed" : "open");
    client.publish("home/gate/log", gateClosed ? "Gate is closed" : "Gate is open");
  }


  if (digitalRead(D0) == LOW && gateClosed)
  {
    gateClosed = false;
    client.publish("home/gated/state", "open");
    client.publish("home/gate/log", "Gate is open");
  }
  else if (digitalRead(D0) == HIGH && !gateClosed)
  {
    client.publish("home/gate/log", "Gate is closed");
    gateClosed = true;
    client.publish("home/gated/state", "closed");
  }



  ArduinoOTA.handle();
  if (!client.connected())
  {
    Serial.println("Not connected, reconnecting");
    reconnect();
  }
  client.loop();
}