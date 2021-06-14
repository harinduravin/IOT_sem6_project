#if defined(ESP8266)
#include <ESP8266WiFi.h>          
#else
#include <WiFi.h>          
#endif

//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h> 

void setup() { // put your setup code here, to run once: 

Serial.begin(115200); 
WiFiManager wifiManager; 
Serial.println("Conecting....."); 
wifiManager.autoConnect("IoT6B_G05","DHST@G05");
Serial.println("connected");
Serial.println(WiFi.localIP());



}

void loop() { // put your main code here, to run repeatedly:
  WiFi.disconnect(true);
  }
