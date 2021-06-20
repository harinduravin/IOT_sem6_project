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
#include <TimeLib.h>                 
#include <LiquidCrystal.h>  
LiquidCrystal lcd(D6, D5, D1, D2, D3, D4); 

ESP8266WebServer server(80);

WiFiClient wifiClient;
PubSubClient client(wifiClient); 
const char* mqttServer = "test.mosquitto.org";

//struct to save user data
uint addr = 0;
struct { 
  bool Authenticated = false;
  bool Data_provided = false;
  String Password = "";
  char Email[50] = "";
  String accesstoken = "";
  }data;

// Setup NTP for time and date
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 19800, 60000);
char Time[ ] = "TIME:00:00:00";
char Date[ ] = "DATE:00/00/2000";
byte last_second, second_, minute_, hour_, day_, month_;
int year_;
int counter = 0;
unsigned long unix_epoch;
char ascii;

String Ceil  = "5";
String Floor = "5";

String payloadstr;
unsigned long timestamp;

#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

//Variables to keep the current selected surrency, it's value and whether it increased or decreased
String Current_currency = "USD";
String Current_value = "198.25";
uint8_t Current_UpDown = true;

//Variable to save current currency, ceil and floor percentage values
String UserNeeds;

//Variable to save the received access_token
String access_token;

String Authorization_Message;
String Email_returned;


float USD = 198.78; // up - true, down - false
float GBP = 274.55; // up - true, down - false
float JPY = 1.80; // up - true, down - false
float AUD = 148.71; // up - true, down - false
float KWD = 659.78; // up - true, down - false
float EUR = 235.48; // up - true, down - false


bool usd_up = false;
bool gbp_up = false;
bool jpy_up = false;
bool aud_up = false;
bool kwd_up = false;
bool eur_up = false;
char * binary;

String access_token_received;
String current_currency;
bool ceil_crossed = false;
bool floor_crossed = false;

//Variables required for buzzer sound
int speakerPin = 13;
int len = 15; // the number of notes
char notes[] = " C C C C C C C C C ";// a space represents a rest
int beats[] = { 1,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int tempo = 300;


void setup_wifi() {
  // Connecting to a WiFi network
  delay(5000);
  WiFiManager wifiManager; 
  wifiManager.autoConnect("IoT6B_G05","12345678");
}

void setupMQTT() {
  //Setting uo MQTT server
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
      // ... and resubscribe
      client.subscribe("IOT_6B/G05/BuzzerNotification");
      client.subscribe("IOT_6B/G05/CommonData");
      client.subscribe("IOT_6B/G05/AuthResponse");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 500 milli seconds");
      // Wait 5 seconds before retrying
      delay(500);
    }
  }
}

void setup() {
  
  lcd.begin(16, 2);                 // Initialize 16x2 LCD Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Time);
  lcd.setCursor(0, 1);
  lcd.print(Date);
  timeClient.begin();

  pinMode(speakerPin, OUTPUT);  // Output pin for buzzer

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
  unix_epoch = timeClient.getEpochTime();    // Get Unix epoch time from the NTP server
  second_ = second(unix_epoch);
  if (last_second != second_) {
 

    minute_ = minute(unix_epoch);
    hour_   = hour(unix_epoch);
    day_    = day(unix_epoch);
    month_  = month(unix_epoch);
    year_   = year(unix_epoch);
 

    Time[12] = second_ % 10 + 48;
    Time[11] = second_ / 10 + 48;
    Time[9]  = minute_ % 10 + 48;
    Time[8]  = minute_ / 10 + 48;
    Time[6]  = hour_   % 10 + 48;
    Time[5]  = hour_   / 10 + 48;


    Date[5]  = day_   / 10 + 48;
    Date[6]  = day_   % 10 + 48;
    Date[8]  = month_  / 10 + 48;
    Date[9]  = month_  % 10 + 48;
    Date[13] = (year_   / 10) % 10 + 48;
    Date[14] = year_   % 10 % 10 + 48;


    lcd.setCursor(0, 0);
    lcd.print(Time);
    lcd.setCursor(0, 1);
    lcd.print(Date);
    last_second = second_;

  }
  delay(500);

  if (counter == 8){
    lcd.clear();
    counter = 0;
    lcd.setCursor(0, 0);
    lcd.print("LKR/USD "+ String(USD));
    lcd.setCursor(0, 1);
    lcd.print("LKR/GBP "+ String(GBP));

    if (usd_up){
      ascii = 0x5e;
      lcd.setCursor(15 , 0);
      lcd.print(ascii);
    } else {
      ascii = 0x76;
      lcd.setCursor(15 , 0);
      lcd.print(ascii);
    }

    if (gbp_up){
      ascii = 0x5e;
      lcd.setCursor(15 , 1);
      lcd.print(ascii);
    } else {
      ascii = 0x76;
      lcd.setCursor(15 , 1);
      lcd.print(ascii);
    }
    delay(2000);

    server.handleClient();
    client.loop();

    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LKR/JPY   "+ String(JPY));
    lcd.setCursor(0, 1);
    lcd.print("LKR/AUD "+ String(AUD));

    if (jpy_up){
      ascii = 0x5e;
      lcd.setCursor(15 , 0);
      lcd.print(ascii);
    } else {
      ascii = 0x76;
      lcd.setCursor(15 , 0);
      lcd.print(ascii);
    }

    if (aud_up){
      ascii = 0x5e;
      lcd.setCursor(15 , 1);
      lcd.print(ascii);
    } else {
      ascii = 0x76;
      lcd.setCursor(15 , 1);
      lcd.print(ascii);
    }
    delay(2000);

    server.handleClient();
    client.loop();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LKR/KWD "+ String(KWD));
    lcd.setCursor(0, 1);
    lcd.print("LKR/EUR "+ String(EUR));

    if (kwd_up){
      ascii = 0x5e;
      lcd.setCursor(15 , 0);
      lcd.print(ascii);
    } else {
      ascii = 0x76;
      lcd.setCursor(15 , 0);
      lcd.print(ascii);
    }

    if (eur_up){
      ascii = 0x5e;
      lcd.setCursor(15 , 1);
      lcd.print(ascii);
    } else {
      ascii = 0x76;
      lcd.setCursor(15 , 1);
      lcd.print(ascii);
    }

    delay(2000);
    server.handleClient();
    client.loop();
    lcd.clear();
  } else {
    counter += 1;
  }

}

//Function to handle HTTP requests
void handlerequest(){

  //Getting data from the EEPROM  
  EEPROM.get(addr,data);

  //Checking if the user is Authenticated
  if (data.Authenticated){

    //Processing data provided by user
    Current_currency = server.arg("currency");
    Update_values();
    Ceil = server.arg("ceil");
    Floor = server.arg("floor");

    //Checking if the current userneeds differ from the previously provided userneeds
    if (UserNeeds != Current_currency +"$"+ Ceil +"$"+ Floor){

      //Processing the provided user needs
      UserNeeds = Current_currency +"$"+ Ceil +"$"+ Floor;
      String UserNeedswithtime = String(unix_epoch)+"$"+ data.accesstoken +"$"+UserNeeds;
      int time_length = String(unix_epoch).length()+ 1;
      int accesstoken_len = (data.accesstoken).length()+1;
      int currency_len = Current_currency.length() + 1;
      int ceil_len = Ceil.length() + 1;
      int floor_len = Floor.length() + 1; 
      int UserNeeds_len = time_length+accesstoken_len+currency_len + ceil_len + floor_len;
      char UserNeeds_array[UserNeeds_len];
      UserNeedswithtime.toCharArray(UserNeeds_array, UserNeeds_len);

      //Publishing the user needs to MQTT server
      client.publish("IOT_6B/G05/UserNeeds", UserNeeds_array );
      }

    //Directing the user to Home page
    server.send(200, "text/html", SendHTML(Current_currency,Current_value,Current_UpDown));
    }
  else{

    //Checking whether user has provided the data(email and password)
    if (!data.Data_provided){
      data.Data_provided = true;
      EEPROM.put(addr,data);
      EEPROM.commit();
      server.send(200, "text/html", UserAuthentication());
    }
    else{
      //Processing the data provided by user
      String Email = server.arg("email");
      String Password = server.arg("password");
  
      char Email_array[50];
      Email.toCharArray(Email_array, 50);
      String UserAuthentication;
      UserAuthentication = String(unix_epoch)+"$"+Email+"$"+Password;
      int Email_len = Email.length() + 1;
      int Password_len = Password.length()+1;
      int time_length = String(unix_epoch).length()+ 1;
      int UserAuthentication_len = time_length+Password_len + Email_len;
      char UserAuthentication_array[UserAuthentication_len];
      UserAuthentication.toCharArray(UserAuthentication_array, UserAuthentication_len);

      //Publishing the data to MQTT server to check whether user data is in the data base
      client.publish("IOT_6B/G05/UserAuth", UserAuthentication_array ); 

      //Waiting for a response from MQTT server  
      Serial.println("Authorization_Message : ");
      Serial.println(Authorization_Message);
      client.loop();
      delay(2000);
      client.loop();
      delay(2000);
      client.loop();
      delay(2000);

      Serial.println("Authorization_Message : ");
      Serial.println(Authorization_Message);

      //Checking if the user is a valid user
      if (Authorization_Message == "success" && Email_returned == Email ){

        //Saving the data of user to EEPROM
        Serial.println("Authenticated");
        data.Authenticated = true;
        data.accesstoken = access_token;
        data.Password = Password;
        strncpy(data.Email,Email_array,50);
        EEPROM.put(addr,data);
        EEPROM.commit();
        server.send(200, "text/html", SendHTML(Current_currency,Current_value,Current_UpDown)); 
        }
      
      //If the user is not a valid user , redirect them to User Authentication page
      else {
        server.send(200, "text/html", UserAuthentication());
        }
      }
    }
}

//Function to handle invalid HTTP requests
void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

//Function to display User Authentication HTML page
String UserAuthentication() {
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

//Function to display home HTML page where we can submit our needs and current selected currency and price of that currency is shown.
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

  if (Currency == "USD"){
  ptr+= "<option value=\"USD\" selected >USD</option>\n";
  ptr+= "<option value=\"JPY\">JPY</option>\n";
  ptr+= "<option value=\"GBP\">GBP</option>\n";
  ptr+= "<option value=\"EUR\">EUR</option>\n";
  ptr+= "<option value=\"KWD\">KWD</option>\n";
  ptr+= "<option value=\"AUD\">AUD</option>\n";
  }
  else if (Currency == "JPY"){
  ptr+= "<option value=\"USD\">USD</option>\n";
  ptr+= "<option value=\"JPY\" selected >JPY</option>\n";
  ptr+= "<option value=\"GBP\">GBP</option>\n";
  ptr+= "<option value=\"EUR\">EUR</option>\n";
  ptr+= "<option value=\"KWD\">KWD</option>\n";
  ptr+= "<option value=\"AUD\">AUD</option>\n";
  }
  else if (Currency == "GBP"){
  ptr+= "<option value=\"USD\">USD</option>\n";
  ptr+= "<option value=\"JPY\">JPY</option>\n";
  ptr+= "<option value=\"GBP\" selected >GBP</option>\n";
  ptr+= "<option value=\"EUR\">EUR</option>\n";
  ptr+= "<option value=\"KWD\">KWD</option>\n";
  ptr+= "<option value=\"AUD\">AUD</option>\n";
  }
  else if (Currency == "EUR"){
  ptr+= "<option value=\"USD\">USD</option>\n";
  ptr+= "<option value=\"JPY\">JPY</option>\n";
  ptr+= "<option value=\"GBP\">GBP</option>\n";
  ptr+= "<option value=\"EUR\" selected >EUR</option>\n";
  ptr+= "<option value=\"KWD\">KWD</option>\n";
  ptr+= "<option value=\"AUD\">AUD</option>\n";
  }
  else if (Currency == "KWD"){
  ptr+= "<option value=\"USD\">USD</option>\n";
  ptr+= "<option value=\"JPY\">JPY</option>\n";
  ptr+= "<option value=\"GBP\">GBP</option>\n";
  ptr+= "<option value=\"EUR\">EUR</option>\n";
  ptr+= "<option value=\"KWD\" selected >KWD</option>\n";
  ptr+= "<option value=\"AUD\">AUD</option>\n";
  }
  else if (Currency == "AUD"){
  ptr+= "<option value=\"USD\">USD</option>\n";
  ptr+= "<option value=\"JPY\">JPY</option>\n";
  ptr+= "<option value=\"GBP\">GBP</option>\n";
  ptr+= "<option value=\"EUR\">EUR</option>\n";
  ptr+= "<option value=\"KWD\">KWD</option>\n";
  ptr+= "<option value=\"AUD\" selected >AUD</option>\n";
  }
  else{
  ptr+= "<option value=\"USD\">USD</option>\n";
  ptr+= "<option value=\"JPY\">JPY</option>\n";
  ptr+= "<option value=\"GBP\">GBP</option>\n";
  ptr+= "<option value=\"EUR\">EUR</option>\n";
  ptr+= "<option value=\"KWD\">KWD</option>\n";
  ptr+= "<option value=\"AUD\">AUD</option>\n";
  }


  ptr+= "</select>\n";
  ptr+= "<br><br>\n";
  ptr+= "<label for=\"ceil\">Ceil% :</label><br>\n";
  ptr+= "<input type=\"number\" id=\"ceil\" name=\"ceil\" value=" + Ceil + "><br><br>\n";
  ptr+= "<label for=\"floor\">Floor% :</label><br>\n";
  ptr+= "<input type=\"number\" id=\"floor\" name=\"floor\" value=" + Floor + "><br><br>\n";
  ptr+= "<input  style=\"height:35px;width: 100px;background-color: aquamarine;\" type=\"submit\" value=\"Submit\">\n";
  ptr+= "</form>\n";
  ptr+= "</body>\n";
  ptr+= "</html>\n";
  return ptr;
}

//Function to handle data received by MQTT server
void callback(char* topic, byte* payload, unsigned int length) {

  if (String(topic) == "IOT_6B/G05/BuzzerNotification") {
   process_notification(payload, length, 50, 5);
  }
  
  if (String(topic) == "IOT_6B/G05/CommonData") {
   process_data(payload, length, 70, 8);
  }

  if (String(topic) == "IOT_6B/G05/AuthResponse") {
   process_Authentication(payload, length, 80, 4);
  }

  EEPROM.get(addr,data);
  if (access_token_received == data.accesstoken){
    if (ceil_crossed  || floor_crossed) {
      buzzerinit();  
      ceil_crossed = false;
      floor_crossed = false;
    }
  }
  else{
    Serial.println("User is not Authenticated")
  }
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
          timestamp = atol(token);
          Serial.print(timestamp);
          Serial.println();
          break;
      case 2:
          if (timestamp > unix_epoch - 19820) {
          Email_returned = String(token);
          Serial.println(Email_returned);
          }
          break;
      case 3:
          if (timestamp > unix_epoch - 19820) {
          Authorization_Message = String(token);
          Serial.println(Authorization_Message);
          }
          break;
      case 4:
          if (timestamp > unix_epoch - 19820) {
          access_token = String(token);
          //saving to eeprom
          data.accesstoken = access_token;
          EEPROM.put(addr,data);
          EEPROM.commit();
          Serial.println(access_token);
          }
          break;
          }          
          token = strtok(NULL, "$");
    }

}

void process_notification(byte* payload, unsigned int length, int charlen, int numitem) {

    int digit;
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
          timestamp = atol(token);
          Serial.print(timestamp);
          Serial.println();
          break;
       case 2:
          if (timestamp > unix_epoch - 19820) {
           access_token_received = String(token);
           Serial.print(access_token_received);
           Serial.println();
          }
           break;
       case 3:
          if (timestamp > unix_epoch - 19820) {
           current_currency = String(token);
           Serial.print(current_currency);
           Serial.println();
          }
           break;
       case 4:
          if (timestamp > unix_epoch - 19820) {
           digit = String(token).toInt();
           if (digit == 1) {
           ceil_crossed = true;
            } else {
            ceil_crossed  = false;
            }
           Serial.print(ceil_crossed);
           Serial.println();
          }
          break;
       case 5:
          if (timestamp > unix_epoch - 19820) {
           digit = String(token).toInt();
           if (digit == 1) {
           floor_crossed = true;
            } else {
           floor_crossed  = false;
            }
           Serial.print(ceil_crossed);
           Serial.println();
          }
          break;    
       }
       token = strtok(NULL, "$");
   }
}

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
          timestamp = atol(token);
          Serial.print(timestamp);
          Serial.println();
          break;
       case 2:
          if (timestamp > unix_epoch - 19820) {
           USD = String(token).toFloat();
           Serial.print(USD);
           Serial.println();
           break;
          }
       case 3:
          if (timestamp > unix_epoch - 19820) {
           GBP = String(token).toFloat();
           Serial.print(GBP);
           Serial.println();
           break;
          }
       case 4:
          if (timestamp > unix_epoch - 19820) {
           JPY = String(token).toFloat();
           Serial.print(JPY);
           Serial.println();
          break;
          }
       case 5:
          if (timestamp > unix_epoch - 19820) {
           AUD = String(token).toFloat();
           Serial.print(AUD);
           Serial.println();
          break;
          }
       case 6:
          if (timestamp > unix_epoch - 19820) {
           KWD = String(token).toFloat();
           Serial.print(KWD);
           Serial.println();
          break;
          }
       case 7:
          if (timestamp > unix_epoch - 19820) {
           EUR = String(token).toFloat();
           Serial.print(EUR);
           Serial.println();
          break;
          }
       case 8:
          if (timestamp > unix_epoch - 19820) {
           set_updown(token);
           break;
          }        
       }
       token = strtok(NULL, "$");
   }
   
}

//Function to update Currency value and updown status based on current currency.
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



void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };

  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void buzzerinit() {
  for (int i = 0; i < len; i++) {
    if (notes[i] == ' ') {
      delay(beats[i] * tempo); // rest
    } else {
      playNote(notes[i], beats[i] * tempo);
    }

    // pause between notes
    delay(tempo / 2); 
  }
}
