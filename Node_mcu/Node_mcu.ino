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
#include <EEPROM.h>

int BUILTIN_LED = 2;
WebServer server(80);

WiFiClient wifiClient;
PubSubClient client(wifiClient); 

const char* mqttServer = "test.mosquitto.org";
uint addr = 0;
struct { 
  bool Authenticated = false;
  char email[30] = "";
  char Email_submitted[30] = "";
  }data;
bool Authorized = false;
 
void setup_wifi() {
  // Connecting to a WiFi network
  delay(5000);
  WiFiManager wifiManager; 
  wifiManager.autoConnect("IoT6B_G05");
}

void setupMQTT() {
  client.setServer(mqttServer,1883);
  client.setCallback(callback);
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
//      client.subscribe("IOT_6B/G05/BuzzerNotification");
      client.subscribe("IOT_6B/G05/CommonData");
      
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
  EEPROM.begin(512);
  setup_wifi();
  setupMQTT();
  data.Authenticated = true;
  strncpy(data.email, "dhanuka37@outlook.com",30);
  EEPROM.put(addr,data);
  EEPROM.commit(); 

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

      String Email = server.arg("email");
      String Password = server.arg("password");
      char Email_array[30];
      Email.toCharArray(Email_array, 30);

      if (!Authorized){
        strncpy(data.Email_submitted, Email_array,30);
        EEPROM.put(addr,data);
        EEPROM.commit();
      }
      
      EEPROM.get(addr,data);
      
      if (strcmp(data.email,data.Email_submitted) == 0){
        if (data.Authenticated){
          Authorized = true;
          String UserNeeds;
          String currency = server.arg("currency");
          String Ceil = server.arg("ceil");
          String Floor = server.arg("floor");
          
          UserNeeds = currency +"$"+ Ceil +"$"+ Floor;
          
          int currency_len = currency.length() + 1;
          int ceil_len = Ceil.length() + 1;
          int floor_len = Floor.length() + 1; 
    
          int UserNeeds_len = currency_len + ceil_len + floor_len;
    
          char UserNeeds_array[UserNeeds_len];
          UserNeeds.toCharArray(UserNeeds_array, UserNeeds_len);
          client.publish("IOT_6B/G05/UserNeeds", UserNeeds_array );
          server.send(200, "text/html", SendHTML(currency));
        }
        else{
          server.send(200, "text/html", UserAuthentification());
        }
      }
      else{
          server.send(200, "text/html", UserAuthentification());
        }
         
//      String UserNeeds;
//      String currency = server.arg("currency");
//      String Ceil = server.arg("ceil");
//      String Floor = server.arg("floor");
//      
//      UserNeeds = currency +"$"+ Ceil +"$"+ Floor;
//      
//      int currency_len = currency.length() + 1;
//      int ceil_len = Ceil.length() + 1;
//      int floor_len = Floor.length() + 1; 
//
//      int UserNeeds_len = currency_len + ceil_len + floor_len;
//
//      char UserNeeds_array[UserNeeds_len];
//      UserNeeds.toCharArray(UserNeeds_array, UserNeeds_len);
//      client.publish("IOT_6B/G05/UserNeeds", UserNeeds_array );
//      server.send(200, "text/html", SendHTML(currency));
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String UserAuthentification() {
  String s="<!DOCTYPE html>\n";
  s+= "<html lang=\"en\">\n";
  s+= "<head>\n";
  s+= "<meta charset=\"UTF-8\">\n";
  s+= "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n";
  s+= "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  s+= "<title>Login</title>\n";
  s+= "</head>\n";
  s+= "<body\n";
  s+= "style=\"text-align:center;\n";
  s+= "display:grid;\n";
  s+= "place-content: center;\n";
  s+= "background-color: azure;\">\n";
  s+= "<h1 style=\" font-size: 45px;\" >Get Exchange</h1>\n";
  s+= "<h2 style=\" font-size: 45px;\" >Sign In</h2>\n";
  s+= "<form method=\"post\" style=\" font-size: xx-large;\" >\n";
  s+= "<label for=\"ceil\">Email :</label><br>\n";
  s+= "<input type=\"email\" id=\"email\" name=\"email\" value=\"\" required><br><br>\n";
  s+= "<label for=\"floor\">Password :</label><br>\n";
  s+= "<input type=\"password\" id=\"password\" name=\"password\" value=\"\" required><br><br>\n";
  s+= "<input style=\"height:35px;width: 100px;background-color: aquamarine;\" type=\"submit\" value=\"Submit\">\n";
  s+= "</form>\n";
  s+= "</body>\n";
  s+= "</html>\n";
  return s;
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
  ptr+= "<body style=\"text-align:center;display:grid;place-content: center;background-color: rgb(23, 196, 196);overflow-y: scroll;\"\n";
  ptr+= "<h1 style=\"font-size: 50px;\" >Get Exchange</h1>\n";
  
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
  
  ptr+= "<h1 style=\" color :rgb(62, 128, 0)\">";
  ptr+= Currency;
  ptr+= "<i class=\"fa fa-arrow-up\"></i></h1>\n";
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

void callback(char* topic, byte* payload, unsigned int length) {

//  if (String(topic) == "IOT_6B/G05/BuzzerNotification") {
//    String payloadstr;
//    Serial.println();
//    for (int i = 0; i < length; i++) {
//      payloadstr += (char)payload[i];
//    }
//    Serial.println(payloadstr);
//     char payloadstr_array[50];
//     payloadstr.toCharArray(payloadstr_array, 50);
//
//   char * token = strtok(payloadstr_array, "$");
//   // loop through the string to extract all other tokens
//   while( token != NULL ) {
//      Serial.print(token); //printing each token
//      Serial.println();
//      token = strtok(NULL, "$");
//   }
//  }

  
//  if (String(topic) == "IOT_6B/G05/CommonData") {
//    String payloadstr;
//    Serial.println();
//    for (int i = 0; i < length; i++) {
//      payloadstr += (char)payload[i];
//    }
//
//     char payloadstr_array[50];
//     payloadstr.toCharArray(payloadstr_array, 70);
//
//   char * token = strtok(payloadstr_array, "$");
//   // loop through the string to extract all other tokens
//   while( token != NULL ) {
//      Serial.print(token); //printing each token
//      Serial.println();
//      token = strtok(NULL, "$");
//   }
//  }
}
