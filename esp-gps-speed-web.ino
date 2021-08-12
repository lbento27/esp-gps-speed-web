/*********************************************************************
LBento
The following is an ESP8266 Digital Speedo based on the Wemos D1 Mini in ap mode as webserver.
The tested GPS module was a U-blox NEO 6M part number GY-GPS6MV2.

IMP NOTE:
To use webserver and gps we can not use the softwareSerial lib, because the cpu only focuses on softserial and wifi ap won't work so we need to use the physical serial ports rx tx
and because of the single core esp8266 we need to only update gps info for 1s otherwise web server will be unresponsive.
Test: try overclock esp8266 to 160MHZ(in arduino ide just set it under tools) or adapt code to esp32 and use the 2 cores (tasks)

Note2: By default Neo6m gps modules come with an update rate of 1000ms and a port rate of 9600, we can change that using a serial programer to connect to gps and using U-center software
See tutorial in "NEO6M configuration" folder, in that case change  Serial.begin(); line of code.
*********************************************************************/

//wemos D1 mini pin out 
 // D7 and D8 , 5v and GND > GPS (wemos D7 to tx on gps module)(wemos D8 to Rx on gps module) OR comment Serial.swap() em Setup to use te RX TX from wemos

//Remove this and 3 more down if you dont want to use favicon
#include <FS.h>   //Include File System Headers upload and uses files/imagens data folder of project(create folder an upload using tools-esp8266 skecthdata upload)https://randomnerdtutorials.com/install-esp8266-filesystem-uploader-arduino-ide/

#include <TinyGPS++.h>                                  // Tiny GPS Plus Library
//webserver lib
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

const char* ssid = "RC-GPS-Bento";
const char* password = "12345678";
/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);
//end webserver lib

TinyGPSPlus gps;                                        // Create an Instance of the TinyGPS++ object called gps

int gps_speed;
int max_speed = 0;
int targetSpeed = 30; //set target speed

int num_sat;

int gTime;//gps time
int h; 
int hh;
int m;
int mm;

int alt;//gps altitude
float lati;
float longi;
String lati_str;
String longi_str;

int time_state = 1;
float tsec=0.0;
String tsec_str="0";
unsigned long sec; 
unsigned long startTime;  
unsigned long endTime;


void setup()   {                
    //IMP
    Serial.begin(9600); //must be 9600 because the gps default (if you change the config of gps, change port rate here to match (115200))
    //IMP
    Serial.swap(); // by calling serial.swap we move rx and tx pins to GPIO15 (TX)D8 and GPIO13 (RX)D7, this is different from softwareserial!
    delay(500);
     
    WiFi.mode(WIFI_AP);           //Only Access point
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);

    //Initialize File System
    SPIFFS.begin();//Remove this if you dont want to use favicon
    
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
// For default 1 second we parse GPS data and report some key values, must be 100 and 1000(best results) otherwise webpage will be unresponsive
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (Serial.available())
    {
      if (gps.encode(Serial.read())){ // Did a new valid sentence come in?

       gpsStatus();    
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
//Note: because gps read updates every one sec we can determine decimal cases, only seconds, possible solution its to update gps config direct see note on top for more info

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
  if(startTime > 0 && time_state == 2){
      sec = endTime-startTime;
        
      if(sec < 0 || sec > 3000){//reset timer in case of bug
         tsec = 0.0;
      }
      
      tsec=(float)sec / 100.0;
      
      tsec_str = String(tsec , 1);//ready to display with 1 decimal case
      //tsec_str = (int)tsec;//pass only sec with no decimal case 
      
      time_state = 3;
  }
}

void gpsStatus(){
  
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
}


//WebServer
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML()); 
}

void handle_reset() {
  max_speed = 0;
  time_state = 1;
  tsec = 0.0;
  tsec_str = "0";
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
  //favicon
  ptr +="<link rel=\"apple-touch-icon\" sizes=\"180x180\" href=\"/apple-icon-180x180.png\">\n";
  ptr +="<link rel=\"icon\" type=\"image/png\" sizes=\"192x192\"  href=\"/android-icon-192x192.png\">\n";
  ptr +="<link rel=\"icon\" type=\"image/png\" sizes=\"32x32\" href=\"/favicon-32x32.png\">\n";
  ptr +="<link rel=\"icon\" type=\"image/png\" sizes=\"96x96\" href=\"/favicon-96x96.png\">\n";
  ptr +="<link rel=\"icon\" type=\"image/png\" sizes=\"16x16\" href=\"/favicon-16x16.png\">\n";
  ptr +="<link rel=\"manifest\" href=\"/manifest.json\">\n";
  ptr +="<meta name=\"theme-color\" content=\"#ffffff\">\n";
  //end favicon
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
  ptr +=tsec_str;
  ptr +=" s</h3>\n";}
  if(time_state == 0){
  ptr +="<h3 style=\"color: #27750f; text-align: right;display: inline-block;margin-left: 100px;\">";
  ptr +="0-";
  ptr +=targetSpeed;
  ptr +="Km/h: ";
  ptr +=tsec_str;
  ptr +=" s</h3>\n";}
  if(time_state == 2){
  ptr +="<h3 style=\" color: #fcf000; text-align: right;display: inline-block;margin-left: 100px;\">";
  ptr +="0-";
  ptr +=targetSpeed;
  ptr +="Km/h: ";
  ptr +=tsec_str;
  ptr +=" s</h3>\n";}
  if(time_state == 3){
  ptr +="<h3 style=\" color: #b80000; text-align: right;display: inline-block;margin-left: 100px;\">";
  ptr +="0-";
  ptr +=targetSpeed;
  ptr +="Km/h: ";
  ptr +=tsec_str;
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
  //ptr +="<h5>";
  //ptr +=startTime;
  //ptr +="    ";
  //ptr +=endTime;
  //ptr +="</h5>\n";
  //end debug
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void handle_NotFound(){
  if(loadFromSpiffs(server.uri())) return;//needed to get imagens working Spiffs FiLe system //Remove this if you dont want to use favicon
  server.send(404, "text/plain", "Not found");
}

//Remove this if you dont want to use favicon
// Handle imagens Spiffs FiLe system 
bool loadFromSpiffs(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";
 
  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }
 
  dataFile.close();
  return true;
}
