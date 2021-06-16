#if defined(ESP8266)
#include <ESP8266WiFi.h>          
#else
#include <WiFi.h>          
#endif

#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif

#include <WiFiManager.h> 
#include <DNSServer.h>
#include <PubSubClient.h>

int BUILTIN_LED = 2;
WebServer server(80);

WiFiClient wifiClient;
PubSubClient client(wifiClient); 
const char* mqttServer = "test.mosquitto.org";

void setup_wifi() {
  // Connecting to a WiFi network
  delay(5000);
  WiFiManager wifiManager; 
  wifiManager.autoConnect("IoT6B_G05");
}

void setupMQTT() {
  client.setServer(mqttServer,1883);
//  client.setCallback(callback);
  }

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("IOT_6B/G05/start", "Hello World");
      // ... and resubscribe
//      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  WiFi.mode(WIFI_AP_STA);
  Serial.begin(115200);
  setup_wifi();
  setupMQTT();
  
  server.on("/",handlerequest);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}

void handlerequest(){
//  if (server.hasArg("plain")== false){ //Check if body received
//      server.send(200, "text/plain", "Body not received");
//      return;
//      }
      String currency = server.arg("currency");
      String Ceil = server.arg("ceil");
      String Floor = server.arg("floor");

      int currency_len = currency.length() + 1;
      int ceil_len = Ceil.length() + 1;
      int floor_len = Floor.length() + 1; 
      char currency_array[currency_len];
      char ceil_array[ceil_len];
      char floor_array[floor_len];
      currency.toCharArray( currency_array, currency_len);
      Ceil.toCharArray( ceil_array, ceil_len);
      Floor.toCharArray( floor_array, floor_len);
      
      client.publish("IOT_6B/G05/currency", currency_array );
      client.publish("IOT_6B/G05/ceil", ceil_array);
      client.publish("IOT_6B/G05/floor", floor_array);
      server.send(200, "text/html", SendHTML(currency));
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(String Currency){
  String ptr = "<!DOCTYPE html> <html lang=\"en\">\n";
  ptr+= "<head>\n";
  ptr+= "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n";   
  ptr+= "<meta charset=\"UTF-8\">\n";
  ptr+= "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n";
  ptr+= "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  ptr+= "<title>GROUP 5</title>\n";
  ptr+= "</head>\n";
  ptr+= "<body style=\"text-align:center;display:grid;place-content: center;background-color: rgb(23, 196, 196);\"\n";
  ptr+= "<h1 style=\"font-size: 200px;\" >Get Exchange</h1>\n";
  
  if (Currency == "USD"){
    ptr+="<h2>LKR/USD</h2>\n";
  }
  else if (Currency == "JPY"){
    ptr+="<h2>LKR/JPY</h2>\n";
  }
  else if (Currency == "GBP"){
    ptr+="<h2>LKR/GBP</h2>\n";
  }
  else if (Currency == "EUR"){
    ptr+="<h2>LKR/EUR</h2>\n";
  }
  else if (Currency == "KWD"){
    ptr+="<h2>LKR/KWD</h2>\n";
  }
  else if (Currency == "INR"){
    ptr+="<h2>LKR/INR</h2>\n";
  }
  else{
    ptr+="<h2>LKR/NON</h2>\n";
  }
  
  ptr+= "<h1 style=\" color :rgb(62, 128, 0)\">1.81 <i class=\"fa fa-arrow-up\"></i></h1>\n";
  ptr+= "<h1 style=\" color :red\">1.81 <i class=\"fa fa-arrow-down\"></i></h1>\n";
  ptr+= "<form name=\"dropdown\" method=\"get\" style=\" font-size: xx-large;\" >\n";
  ptr+= "<label for=\"currency_label\">Select Currency :</label><br>\n";
  ptr+= "<select name=\"currency\" id=\"currency\">\n";
  ptr+= "<option value=\"USD\">USD</option>\n";
  ptr+= "<option value=\"JPY\">JPY</option>\n";
  ptr+= "<option value=\"GBP\">GBP</option>\n";
  ptr+= "<option value=\"EUR\">EUR</option>\n";
  ptr+= "<option value=\"KWD\">KWD</option>\n";
  ptr+= "<option value=\"INR\">INR</option>\n";
  ptr+= "</select>\n";
  ptr+= "<br><br>\n";
  ptr+= "<label for=\"ceil\">Ceil% :</label><br>\n";
  ptr+= "<input type=\"number\" id=\"ceil\" name=\"ceil\" value=5><br><br>\n";
  ptr+= "<label for=\"floor\">Floor% :</label><br>\n";
  ptr+= "<input type=\"number\" id=\"floor\" name=\"floor\" value=5><br><br>\n";
  ptr+= "<input type=\"submit\" value=\"Submit\">\n";
  ptr+= "</form>\n";
  ptr+= "</body>\n";
  ptr+= "</html>\n";
  return ptr;
}
