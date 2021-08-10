/*********************************************************************
LBento
The following is an ESP8266 Digital Speedo based on the Wemos D1 Mini in ap mode as webserver.
The tested GPS module was an U-blox NEO 6M part number GY-GPS6MV2.

IMP NOTE:
to use webserver and gps we can not use the softwareSerial lib, because the cpu only focus on softserial and wifi ap wont work so we need to use the fisical serial ports rx tx
*********************************************************************/

//wemos D1 mini pin out 
 // D7 and D8 , 5v and GND > GPS (wemos D7 to tx on gps module)(wemos D8 to Rx on gps module) OR comment Serial.swap() em Setup to use te RX TX from wemos


#include <TinyGPS++.h>                                  // Tiny GPS Plus Library

//webserver lib
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "RC-GPS-Bento";
const char* password = "12345678";
/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);
//end webserver lib

int gps_speed;
int num_sat;
int max_speed = 0;
int time_state = 1;

int mil;
float mil2;
int tsec; 
uint32_t startMillis;  //some global variables available anywhere in the program
uint32_t currentMillis;

int targetSpeed = 30; //set target speed for 0 to 
int incrementState = LOW;


TinyGPSPlus gps;                                        // Create an Instance of the TinyGPS++ object called gps

void setup()   {                

    Serial.begin(9600); //must be 9600 because gps
    Serial.swap(); // by calling serial.swap we move de rx an tx pins to GPIO15 (TX)D8 and GPIO13 (RX)D7, this is diferent from softwareserial!
    delay(500);
    
    pinMode(LED_BUILTIN, OUTPUT); //debug
    
    WiFi.mode(WIFI_AP);           //Only Access point
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);
    
    server.on("/", handle_OnConnect);
    server.on("/reset", handle_reset);
    server.on("/read", handle_read);
    server.onNotFound(handle_NotFound);
    
    server.begin();  
  }




void loop() {
  server.handleClient();
  gpsRead();
  }

//WebServer
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML()); 
}

void handle_reset() {
  max_speed = 0;
  time_state = 1;
  tsec = 0;
  server.send(200, "text/html", SendHTML()); 
}

void handle_read() {
  server.send(200, "text/html", SendHTML()); 
}

String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>RC GPS</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>RC GPS</h1>\n";
  ptr +="<h3>SAT:</h3>\n";
  ptr +="<h2>";
  ptr +=num_sat;
  ptr +="</h2>\n";
  ptr +="<h3>Max Speed:</h3>\n";
  ptr +="<h2>";
  ptr +=max_speed;
  ptr +="Km/h</h2>\n";
  ptr +="<a class=\"button button-off\" href=\"/reset\">RESET</a>\n";
  ptr +="<a class=\"button button-off\" href=\"/read\">Read</a>\n";
  ptr +="<h4>Lbento</h4>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}


//GPS FUNC
void gpsSpeed(){
  
  num_sat = gps.satellites.value();
  gps_speed = gps.speed.kmph();  

  //Max speed calc
  if (gps_speed > max_speed){
    max_speed = gps_speed;
  }
 
  //0-30km/h calc
  if (gps_speed > 1 && time_state == 1){
    startMillis = millis();
    time_state = 0;
  } 
  if (gps_speed >= targetSpeed && time_state == 0){ //change where if you want to call to a different speed
    currentMillis = millis();
    //calculate seconds with decimal case for more acurate mesure
    tsec = (currentMillis - startMillis)/1000;
    mil = (currentMillis - startMillis)%1000;
    mil2 = ((currentMillis - startMillis) / 1000.0, 1);
    if (mil < 100){
    mil = 0;
    }
    if (mil < 10){
    mil = 0;
    }
        
    time_state = 2;
  }
  //reset timer in case of bug
  if(tsec > 60){
    tsec=0;
  }

}

void gpsRead(){

// For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (Serial.available())
    {
      if (gps.encode(Serial.read())) // Did a new valid sentence come in?
       gpsSpeed();
    }
  }
  
  }
