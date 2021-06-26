/*
 *    Configuration file for ESP Gate
 */

// Replace with your network credentials
const char* ssid     = "YOURSSID";
const char* password = "PASSWORD";

const int outputPin = 4; //<<-- The GPIO on which is connected the gate switch

const bool pullup = 1; //<<-- Put 1 if the port should be normally at 5V-HIGH (5v nothing, GND to Open) 
                       //     Put 0 if the port should be normally at GND-LOW (GND nothing, 5V to Open)

// MQTT Server address
const char* mqtt_server = "192.168.1.46";//<<-- Your MQTT server IP