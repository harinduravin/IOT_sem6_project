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
#include <WiFiUdp.h>
#include <NTPClient.h>    

int BUILTIN_LED = 2;
WebServer server(80);

WiFiClient wifiClient;
PubSubClient client(wifiClient); 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);


const char* mqttServer = "test.mosquitto.org";
uint addr = 0;
struct { 
  bool Authenticated = false;
  bool Data_provided = false;
  String Password = "";
  char Email[50] = "";
  String UserID;
  }data;
  


String payloadstr;
long timestamp;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
String Current_currency = "NON";
String Current_value = "0.00";
uint8_t Current_UpDown = true;
String UserNeeds;
unsigned long unix_epoch;
String User_ID;
String Authorization_Message;
String Email_returned;


float USD = 198.25; // up - true, down - false
float GBP = 198.25; // up - true, down - false
float JPY = 198.25; // up - true, down - false
float AUD = 198.25; // up - true, down - false
float KWD = 198.25; // up - true, down - false
float EUR = 198.25; // up - true, down - false


bool usd_up = false;
bool gbp_up = false;
bool jpy_up = false;
bool aud_up = false;
bool kwd_up = false;
bool eur_up = false;
char * binary;
 
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
      client.subscribe("IOT_6B/G05/AuthResponse");
      
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
  timeClient.begin();
  setup_wifi();
  setupMQTT();
  data.Authenticated = false;
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
  timeClient.update();
  unix_epoch = timeClient.getEpochTime();
}



void handlerequest(){
      
      String Email = server.arg("email");
      String Password = server.arg("password");

      char Email_array[50];
      Email.toCharArray(Email_array, 50);
     
      EEPROM.get(addr,data);
      
      if (data.Authenticated){
           
        Current_currency = server.arg("currency");
        Update_values();
        server.send(200, "text/html", SendHTML(Current_currency,Current_value,Current_UpDown));
     
        String Ceil = server.arg("ceil");
        String Floor = server.arg("floor");

        if (UserNeeds != Current_currency +"$"+ Ceil +"$"+ Floor){
          UserNeeds = Current_currency +"$"+ Ceil +"$"+ Floor;
          String UserNeedswithtime = String(unix_epoch)+"$"+User_ID+"$"+UserNeeds;
          int time_length = String(unix_epoch).length()+ 1;
          int UserId_len = User_ID.length()+1;
          int currency_len = Current_currency.length() + 1;
          int ceil_len = Ceil.length() + 1;
          int floor_len = Floor.length() + 1; 
          int UserNeeds_len = time_length+UserId_len+currency_len + ceil_len + floor_len;
          char UserNeeds_array[UserNeeds_len];
          UserNeedswithtime.toCharArray(UserNeeds_array, UserNeeds_len);
          client.publish("IOT_6B/G05/UserNeeds", UserNeeds_array );
          }
        }
        else{
          if (!data.Data_provided){
            data.Data_provided = true;
            EEPROM.put(addr,data);
            EEPROM.commit();
            server.send(200, "text/html", UserAuthentification());
          }
          else{
            String UserAuthentication;
            UserAuthentication = String(unix_epoch)+"$"+Email+"$"+Password;
            int Email_len = Email.length() + 1;
            int Password_len = Password.length()+1;
            int time_length = String(unix_epoch).length()+ 1;
            int UserAuthentication_len = time_length+Password_len + Email_len;
            char UserAuthentication_array[UserAuthentication_len];
            UserAuthentication.toCharArray(UserAuthentication_array, UserAuthentication_len);
            client.publish("IOT_6B/G05/UserAuth", UserAuthentication_array );
            delay(10000);
            if (Authorization_Message == "success" && Email_returned == Email ){
              Serial.println("HelloAuthorized");
              data.Authenticated = true;
              data.UserID = User_ID;
              data.Password = Password;
              strncpy(data.Email,Email_array,50);
              EEPROM.put(addr,data);
              EEPROM.commit();
              server.send(200, "text/html", SendHTML(Current_currency,Current_value,Current_UpDown)); 
              }
            else{
              data.Data_provided = false;
              EEPROM.put(addr,data);
              EEPROM.commit();
//              server.send(200, "text/html", UserAuthentification());
              }
            }
          }
        }

void handle_NotFound(){
  server.send(404, "text/html", "Not found");
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

String SendHTML(String Currency,String value,uint8_t UpDown){
  String ptr = "<!DOCTYPE html> <html lang=\"en\">\n";
  ptr+= "<head>\n";
  ptr+= "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n";   
  ptr+= "<meta charset=\"UTF-8\">\n";
  ptr+= "<meta http-equiv=\"refresh\" content=\"15\">\n";
  ptr+= "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  ptr+= "<title>GROUP 5</title>\n";
  ptr+= "</head>\n";
  ptr+= "<body style=\"text-align:center;display:grid;place-content: center;background-color: rgb(23, 196, 196);overflow-y: auto;\">\n";
  ptr+= "<h1 style=\"font-size: 45px;\" >Get Exchange</h1>\n";
  
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
  else if (Currency == "AUD"){
    ptr+="<h2>LKR/AUD</h2>\n";
  }
  else{
    ptr+="<h2>LKR/NON</h2>\n";
  }

  if(UpDown){
  ptr+= "<h1 style=\" color :rgb(62, 128, 0)\">";
  ptr+= value;
  ptr+= "<i class=\"fa fa-arrow-up\"></i></h1>\n";
  }
  else{
  ptr+= "<h1 style=\" color :red\">\n";
  ptr+= value;
  ptr+= "<i class=\"fa fa-arrow-down\"></i></h1>\n";
  }
  ptr+= "<form name=\"dropdown\" method=\"get\" style=\" font-size: xx-large;\" >\n";
  ptr+= "<label for=\"currency_label\">Select Currency :</label><br>\n";
  ptr+= "<select name=\"currency\" id=\"currency\">\n";
  ptr+= "<option value=\"USD\">USD</option>\n";
  ptr+= "<option value=\"JPY\">JPY</option>\n";
  ptr+= "<option value=\"GBP\">GBP</option>\n";
  ptr+= "<option value=\"EUR\">EUR</option>\n";
  ptr+= "<option value=\"KWD\">KWD</option>\n";
  ptr+= "<option value=\"AUD\">AUD</option>\n";
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
//    process_notification(payload, length, 50, 5);
//  }
  
  if (String(topic) == "IOT_6B/G05/CommonData") {
   process_data(payload, length, 70, 8);
  }

  if (String(topic) == "IOT_6B/G05/AuthResponse") {
   process_Authentication(payload, length, 50, 5);
  }

//  if (ceil_crossed  || floor_crossed) {
//    buzzerinit();  
//    ceil_crossed = false;
//    floor_crossed = false;
//  }
  
}

void process_Authentication(byte* payload, unsigned int length, int charlen, int numitem) {

    payloadstr = "";
    Serial.println();
    for (int i = 0; i < length; i++) {
      payloadstr += (char)payload[i];
    }

     char payloadstr_array[charlen];
     payloadstr.toCharArray(payloadstr_array, charlen);

   char * token = strtok(payloadstr_array, "$");

   for (int i = 1; i < numitem+1; i++) {
    switch (i) {
      case 1:
          Email_returned = String(token);
          Serial.println(Email_returned);
          break;
      case 2:
          Authorization_Message = String(token);
          Serial.println(Authorization_Message);
          break;
      case 3:
          User_ID = String(token);
          Serial.println(User_ID);
          break;
          }          
          token = strtok(NULL, "$");
    }

}

//void process_notification(byte* payload, unsigned int length, int charlen, int numitem) {
//
//    int digit;
//    payloadstr = "";
//    Serial.println();
//    for (int i = 0; i < length; i++) {
//      payloadstr += (char)payload[i];
//    }
//
//     char payloadstr_array[charlen];
//     payloadstr.toCharArray(payloadstr_array, charlen);
//
//   char * token = strtok(payloadstr_array, "$");
//   
//   for (int i = 1; i < numitem+1; i++) {
//      switch (i) {
//       case 1:
//          timestamp = atol(token);
//          Serial.print(timestamp);
//          Serial.println();
//          break;
//       case 2:
//          if (timestamp > unix_epoch - 19820) {
//           current_user = String(token);
//           Serial.print(current_user);
//           Serial.println();
//          }
//           break;
//       case 3:
//          if (timestamp > unix_epoch - 19820) {
//           current_currency = String(token);
//           Serial.print(current_currency);
//           Serial.println();
//          }
//           break;
//       case 4:
//          if (timestamp > unix_epoch - 19820) {
//           digit = String(token).toInt();
//           if (digit == 1) {
//           ceil_crossed = true;
//            } else {
//            ceil_crossed  = false;
//            }
//           Serial.print(ceil_crossed);
//           Serial.println();
//          }
//          break;
//       case 5:
//          if (timestamp > unix_epoch - 19820) {
//           digit = String(token).toInt();
//           if (digit == 1) {
//           floor_crossed = true;
//            } else {
//           floor_crossed  = false;
//            }
//           Serial.print(ceil_crossed);
//           Serial.println();
//          }
//          break;    
//       }
//       token = strtok(NULL, "$");
//   }
//}

void process_data(byte* payload, unsigned int length, int charlen, int numitem) {

    payloadstr = "";
    Serial.println();
    for (int i = 0; i < length; i++) {
      payloadstr += (char)payload[i];
    }

     char payloadstr_array[charlen];
     payloadstr.toCharArray(payloadstr_array, charlen);

   char * token = strtok(payloadstr_array, "$");

   for (int i = 1; i < numitem+1; i++) {
      switch (i) {
       case 1:
          timestamp = long(token);
          Serial.print(timestamp);
          Serial.println();
          break;
       case 2:

           USD = String(token).toFloat();
           Serial.print(USD);
           Serial.println();
           break;
       case 3:

           GBP = String(token).toFloat();
           Serial.print(GBP);
           Serial.println();
           break;
       case 4:

           JPY = String(token).toFloat();
           Serial.print(JPY);
           Serial.println();
          break;
       case 5:

           AUD = String(token).toFloat();
           Serial.print(AUD);
           Serial.println();
          break;
       case 6:
          //do something when var equals 1
           KWD = String(token).toFloat();
           Serial.print(KWD);
           Serial.println();
          break;
       case 7:

           EUR = String(token).toFloat();
           Serial.print(EUR);
           Serial.println();
          break;
       case 8:
           set_updown(token);
           break;        
       }
       token = strtok(NULL, "$");
   }
   
//   Update_values();
   
}

void Update_values(){

  if (Current_currency == "USD"){
    Current_value = String(USD,2);
    Current_UpDown = usd_up;
   }
   if (Current_currency == "AUD"){
    Current_value = String(AUD,2);
    Current_UpDown = aud_up;
   }
   if (Current_currency == "JPY"){
    Current_value = String(JPY,2);
    Current_UpDown = jpy_up;
   }
   if (Current_currency == "GBP"){
    Current_value = String(GBP,2);
    Current_UpDown = gbp_up;
   }
   if (Current_currency == "KWD"){
    Current_value = String(KWD,2);
    Current_UpDown = kwd_up;
   }
   if (Current_currency == "EUR"){
    Current_value = String(EUR,2);
    Current_UpDown = eur_up;
   }
   if (Current_currency == "NON")
   {
    Current_value = "0.00";
    Current_UpDown = true;
   }

   Serial.println(Current_currency+" "+Current_value);
   
  }

void set_updown(char * binary) {
  int digit;
  for (int i = 1; i < 7; i++) {
    switch (i) {
       case 1:
       digit = String((char)binary[i-1]).toInt();
       if (digit == 1) {
        usd_up = true;
       } else {
        usd_up = false;
       }
       Serial.print(usd_up);
       Serial.println();
       break;

       case 2:
       digit = String((char)binary[i-1]).toInt();
       if (digit == 1) {
        gbp_up = true;
       } else {
        gbp_up = false;
       }
       Serial.print(gbp_up);
       Serial.println();
       break;

       case 3:
       digit = String((char)binary[i-1]).toInt();
       if (digit == 1) {
        jpy_up = true;
       } else {
        jpy_up = false;
       }
       Serial.print(jpy_up);
       Serial.println();
       break;

       case 4:
       digit = String((char)binary[i-1]).toInt();
       if (digit == 1) {
        aud_up = true;
       } else {
        aud_up = false;
       }
       Serial.print(aud_up);
       Serial.println();
       break;

       case 5:
       digit = String((char)binary[i-1]).toInt();
       if (digit == 1) {
        kwd_up = true;
       } else {
        kwd_up = false;
       }
       Serial.print(kwd_up);
       Serial.println();
       break;

       case 6:
       digit = String((char)binary[i-1]).toInt();
       if (digit == 1) {
        eur_up = true;
       } else {
        eur_up = false;
       }
       Serial.print(eur_up);
       Serial.println();
       break;
    }
  }
}
