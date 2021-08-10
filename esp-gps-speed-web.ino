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

TinyGPSPlus gps;                                        // Create an Instance of the TinyGPS++ object called gps

int targetSpeed = 30; //set target speed for 0 to 

int gps_speed;
int num_sat;
int max_speed = 0;

int alt;//gps altitude
int gTime;//gps time
int h; 
int hh;
int m;
int mm;
float lati;
float longi;
String lati_str;
String longi_str;

int sec=0;
float tsec=0.0;

int time_state = 1; 
int startTime;  //some global variables available anywhere in the program
int endTime;


void setup()   {                

    Serial.begin(9600); //must be 9600 because gps
    Serial.swap(); // by calling serial.swap we move de rx an tx pins to GPIO15 (TX)D8 and GPIO13 (RX)D7, this is diferent from softwareserial!
    delay(500);
     
    WiFi.mode(WIFI_AP);           //Only Access point
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);
    
    server.on("/", handle_OnConnect);
    server.on("/reset", handle_reset);
    server.on("/read", handle_read);
    server.on("/target", handle_target);
    server.onNotFound(handle_NotFound);
    
    server.begin();  
  }


void loop() {
  server.handleClient();
  gpsRead();
  }


//GPS FUNC
void gpsRead(){
// For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (Serial.available())
    {
      if (gps.encode(Serial.read())){ // Did a new valid sentence come in?
       
       num_sat = gps.satellites.value();
        
       gTime = gps.time.value();//convert gtime(hhmmsscc) to retrive hh and mm
       h=(gTime / 10000000U) % 10;
       hh=(gTime / 1000000U) % 10;
       m=(gTime / 100000U) % 10;
       mm=(gTime / 10000U) % 10;

       //just for fun, why not
       alt = gps.altitude.meters();
       lati=gps.location.lat();
       longi=gps.location.lng();
       lati_str =String(lati , 6);
       longi_str=String(longi , 6);
       
       gpsMaxSpeed();
       gpsSpeedTime();
      }
    }
  }
}

void gpsMaxSpeed(){
  
  gps_speed = gps.speed.kmph();  

  //Max speed calc
  if (gps_speed > max_speed){
    max_speed = gps_speed;
  }
 
}

void gpsSpeedTime(){

  //0-tagetSpeedkm/h calc
  if (gps_speed > 1 && time_state == 1){
    startTime = gps.time.value();
    time_state = 0;
  } 
  if (gps_speed >= targetSpeed && time_state == 0){ 
    endTime = gps.time.value();
    time_state = 2;
  }
  //calculate time base on gps time
  sec = endTime-startTime;
  tsec=(float)sec / 100.0;
  
  //reset timer in case of bug menor que 0 sec ou maior que 600sec
  if(sec < 0 || sec > 6000){
    tsec = 0.0;
  }
  }

//WebServer
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML()); 
}

void handle_reset() {
  max_speed = 0;
  time_state = 1;
  tsec = 0.0;
  startTime = 0;
  endTime = 0;
  server.send(200, "text/html", SendHTML()); 
}

void handle_target() {
  //change target speed
  if((targetSpeed>=10)&&(targetSpeed<=100)){
    targetSpeed=targetSpeed + 10;
    }
  if(targetSpeed>100){targetSpeed=10;}
  
  server.send(200, "text/html", SendHTML()); 
}


void handle_read() {
  //gpsRead();
  server.send(200, "text/html", SendHTML()); 
}

//ptr +="\n";

String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>RC GPS</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 30px;} h1 {color: #444444;margin: 10px auto 20px;} h3 {color: #444444;}\n";
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
  //info num sat
  if(num_sat < 6){ptr +="<h3 style=\" color:#b80000;text-align: left;display: inline-block;\">";
  ptr +="SAT: ";
  ptr +=num_sat;
  ptr +="</h3>";}
  if(num_sat >= 6){ptr +="<h3 style=\"text-align: left;display: inline-block;\">";
  ptr +="SAT: ";
  ptr +=num_sat;
  ptr +="</h3>";}
  //change color base on timer start(green), timer stop(red), timer ready(black)
  if(time_state == 1){
  ptr +="<h3 style=\"text-align: right;display: inline-block;margin-left: 100px;\">";
  ptr +="0-";
  ptr +=targetSpeed;
  ptr +="Km/h: ";
  ptr +=String(tsec , 1);
  ptr +=" s</h3>\n";}
  if(time_state == 0){
  ptr +="<h3 style=\"color: #27750f; text-align: right;display: inline-block;margin-left: 100px;\">";
  ptr +="0-";
  ptr +=targetSpeed;
  ptr +="Km/h: ";
  ptr +=String(tsec , 1);
  ptr +=" s</h3>\n";}
  if(time_state == 2){
  ptr +="<h3 style=\" color: #b80000; text-align: right;display: inline-block;margin-left: 100px;\">";
  ptr +="0-";
  ptr +=targetSpeed;
  ptr +="Km/h: ";
  ptr +=String(tsec , 1);//ready to display with 1 decimal case;
  ptr +=" s</h3>\n";}
  //end color change
  ptr +="<h3>Max Speed:</h3>";
  ptr +="<h2 style=\"color: #cf0000;\">";
  ptr +=max_speed;
  ptr +="</h2>";
  ptr +="<h3>Km/h</h3>\n";
  ptr +="<a style=\"display: inline-block;\" class=\"button button-off\" href=\"/reset\">RESET</a>\n";
  ptr +="<a style=\"display: inline-block;\" class=\"button button-off\" href=\"/target\">TargS</a>\n";
  ptr +="<a class=\"button button-off\" href=\"/read\">READ</a>\n";
  ptr +="<h4 style=\"text-align: left;display: inline-block;\">Altitude: ";
  ptr +=alt;
  ptr +="m</h4>\n";
  ptr +="<h4 style=\"text-align: right;display: inline-block;margin-left: 100px;\">Time: ";
  ptr +=h;
  ptr +=hh;
  ptr +=":";
  ptr +=m;
  ptr +=mm;
  ptr +="</h4>\n";
  ptr +="<h5>Lat:";
  ptr +=lati_str;
  ptr +=" Lng:";
  ptr +=longi_str;
  ptr +="</h5>\n";
  ptr +="<h5>Inf:For best results wait for 7 sat or more.</h5>\n";
  ptr +="<h5>Lbento</h5>\n";
  //debug
  ptr +="<h5>";
  ptr +=startTime;
  ptr +="    ";
  ptr +=endTime;
  ptr +="</h5>\n";
  //end debug
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}
