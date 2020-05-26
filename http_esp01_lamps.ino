//16-02-2020  Pilotaggio relè via WiFi da ESP01 utilizzando ArduinoIDE solo per la programmazione della scheda 
//ESP8266 generic.
//Started on 20-04-2017
//compilated with Arduino IDE 1.6.5
//Vedi esempio WiFiWebServer
//-181230 Aggiunta gestione stato rele in EEprom, tutorial su 
//    https://arduino.stackexchange.com/questions/25945/how-to-read-and-write-eeprom-in-esp8266
//Vedi esempio  https://thingsboard.io/docs/samples/esp8266/gpio/
//ThingsBoard tutorials:
/* 200215 rivista gestione di IFTTT event 
*/
//      https://www.youtube.com/watch?v=UJydvpwdvzM
/* 200107   Aggiunta gestione via MQTT di Adafruit
  
  username: attilio26
  AIOkey:   d2e4ac1319b6451597dba14376d3d647
  Feeds:    ESP01_2LampsTLC,  ESP01_2LampsLAMP
*/
 
/* Procedura per la programmazione:
    a)  Con IDE Arduino 1.6.5 selezionare la scheda ESP8266 generic.
        (sotto compare 
          Generic ESP8266 Module, 80 Mhz, 40Mhz, DIO, 115200, 512K (64K SPIFFS), ok, Disabled, None on COMx.
    b)  Collegare il modulo ESP01 al programmatore con interfaccia CP2102
    c)  Dare alimentazione alla scheda programmatore da presa USB tenendo a massa il pin GPIO0 PER 
          TUTTA LA FASE DI PROGRAMMAZIONE.
          (collegare uno spezzone a coccodrilli tra il cavo nero scorticato che va al CP2102 e il
          reoforo della resistenza sulla scheda).
    d)  Avviare la progrqammazione.
        Al termine della programmazione il programma parte anche se GPIO0 è ancora tenuto a massa.
    e)  Riportare il modulo ESP01 sull'hardware dove serve.
 */
//Per il programmatore acquistato da Amazon:
//  https://www.youtube.com/watch?v=6uaIWZCRSz8
 
/* Poichè la scheda continua a bloccarsi, penso di farle fare un ping verso il router,
 *  che in timeout deve resettare la scheda stessa.
 *  http://www.esp8266.com/viewtopic.php?p=41196#p41196
 * 

extern "C" {
#include "ping.h"
}
eliminato in 26-10-2017
 */

/*
 * http://www.settorezero.com/wordpress/il-modulo-esp8266-e-il-nodemcu-devkit-parte-2-controlliamo-4-rele-tramite-la-rete-wi-fi-di-casa/
 * https://www.reboot.ms/forum/threads/interruttore-led-su-esp8266-controllato-via-wifi.391/
 * http://mancusoa74.blogspot.it/2015/09/5v-relay-controllato-via-internet-basso.html
 * https://openhomeautomation.net/control-relay-anywhere-esp8266/
 * http://www.mcmajan.com/mcmajanwpr/blog/2017/03/07/esp8266-termometro-wifi-it/
 * -->   https://www.sgart.it/IT/elettro/apertura-cencello-iot-controllato-via-wi-fi-esp8266/post 
 * https://links2004.github.io/Arduino/d4/dd2/class_esp_class.html  (EspClass Class Reference)
 */
 
//The blue LED on the ESP-01 module is connected to GPIO1 which is also the TXD pin. We use uses LED_BUILTIN 
//to find the pin with the internal LED

//https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/readme.md
//https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.md#setautoreconnect

//-------------------Defines
#define rele1Broken             //Il contatto NO del rele1 è danneggiato; si opera con il NC in logica inversa
#define SERIAL_DEBUG
#define EEsize 32               //bytes reserved to EEprom data
#define myIFTTTkey "iRsU-c2LcNtn4HX6ZSD_BAiJpy6fYs7OSphOb7b2sc1"		//account: atiliomelucci@gmail.com
#define myIFTTTevent "ESPlamps"
//#define ThingsBoardToken "zNqEcomGIUIG2FS6RST0"
//To fire my event: https://maker.ifttt.com/trigger/ESPlamps/with/key/iRsU-c2LcNtn4HX6ZSD_BAiJpy6fYs7OSphOb7b2sc1
//---for Adafruit
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "attilio26"
#define AIO_KEY         "d2e4ac1319b6451597dba14376d3d647"
#define CycleTime 10000      //durata ms del ciclo principale


//-------------------Included Librarie 
#include <ESP8266WiFi.h>
//gestione della EEprom https://www.arduino.cc/en/Tutorial/EEPROMWrite
#include <EEPROM.h>             //the EEPROM sizes are powers of two
//IFTTT connection:
//http://www.instructables.com/id/ESP8266-to-IFTTT-Using-Arduino-IDE/
//https://github.com/mylob/ESP-To-IFTTT
//https://github.com/mylob/ESP-To-IFTTT
#include "DataToMaker.h"   //in questa stessa cartella
//Thingsboard connection
//#include <ArduinoJson.h>
//#include <PubSubClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


//-------------------collegamenti dei devices


//-------------------global variables declarations
const char* ssid = "2znirpUSN";
const char* password = "ta269179";
#define tbTOKEN "zNqEcomGIUIG2FS6RST0"    //for Thingsboard
int8_t Val2, Val1;    	// Per le condizioni 0,1 su GPIO2 e GPIO1 rispettivamente
int  MemRele;           //Aux Memoria stato rele in EEprom
String Stati[6] = {"All OFF","Lamp OFF","Lamp ON","Tlc OFF","Tlc ON","All ON"};
char thingsboardServer[] = "demo.thingsboard.io";
unsigned long StartTime;
bool ADAflag = false; 


//-------------------Classes
// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
WiFiClient Iclient;
// declare new maker event with the name "KeyThief"
DataToMaker MyIftttEvent(myIFTTTkey, myIFTTTevent);     //for IFTTT pourpose
//PubSubClient client(Iclient);                         //for Thingsboard MQTT pourpose

//****Adafruit MQTT
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&Iclient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//---PUBLISH to send analog inputs
// Setup a feed called 'wemos232_temp' for publishing.
//Adafruit_MQTT_Publish wemos232_temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/wemos232_temp");
//---SUBSCRIBE to receive commands from web
// telecamera varanda
Adafruit_MQTT_Subscribe ESP01_2LampsTLC = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/esp01-2lampstlc");
// lampada giardino
Adafruit_MQTT_Subscribe ESP01_2LampsLAMP = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/esp01-2lampslamp");
//****

void setup() {
// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
  void MQTT_connect();
// commit EEsize bytes of ESP8266 flash (for "EEPROM" emulation)
// this step actually loads the content (EEsize bytes) of flash into a EEsize-byte-array cache in RAM
  EEPROM.begin(EEsize);
  EEPROM.get(0,MemRele);
// Initialize the LED_BUILTIN pin as an output avoiding the use of Serial.print
  pinMode(LED_BUILTIN, OUTPUT); 
  digitalWrite(LED_BUILTIN, bitRead(MemRele,0));   //The blue led is ON, but LOW is the voltage level 
// prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, bitRead(MemRele,1));
  
  // Connect to WiFi network
  debug("Connecting to ");
  debugln(ssid);
//Init WiFi connection
  /*  
   * Viene impostato il modo station (differente da AP o AP_STA) 
   * La modalità STA consente all'ESP8266 di connettersi a una rete Wi-Fi 
   * (ad esempio quella creata dal router wireless), mentre la modalità AP  
   * consente di creare una propria rete e di collegarsi 
   * ad altri dispositivi (ad esempio il telefono). 
   */  
  WiFi.mode(WIFI_STA);  
  /*  
   *  Inizializza le impostazioni di rete della libreria WiFi e fornisce lo stato corrente della rete, 
   *  nel caso in esempio ha come parametri ha il nome della rete e la password. 
   *  Restituisce come valori: 
   *   
   *  WL_CONNECTED quando connesso al network 
   *  WL_IDLE_STATUS quando non connesso al network, ma il dispositivo è alimentato 
  */  
  WiFi.begin(ssid, password); 
//Impostazioni wifi: Commentare se si usa il dhcp del router)
//WiFi.config(ip,gateway, subnet);
  WiFi.config(IPAddress(192,168,4,21), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
/*  0 : WL_IDLE_STATUS      1 : WL_NO_SSID_AVA      3 : WL_CONNECTED
    4 : WL_CONNECT_FAILED   6 : WL_DISCONNECTED
*/
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    debugc('.');
  }
  debugln();
  debugln("WiFi connected");
//Start Server
  server.begin();                    
  debugln("Server started");
// Print the IP address
  Serial.println(WiFi.localIP());
  /* for wdog manipulation
  ESP.wdtDisable();
  ESP.wdtEnable();
  ESP.wdtFeed();
*/
  ESP.wdtEnable(6000);
// Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&ESP01_2LampsLAMP);
  mqtt.subscribe(&ESP01_2LampsTLC);
  StartTime = millis();
}


void loop() {
//Returns true if Station is connected to an access point or false if not.
  if(!WiFi.isConnected()){          
    ESP.restart();     
    return;
  }
//ADAflag commuta HIGH LOW per gestire http request ed MQTT request  
  if((millis() - StartTime) > CycleTime){
    ADAflag = !ADAflag;
    StartTime = millis();
  }

  if(ADAflag == false){
    ESP.wdtFeed();
    // Check if a client has connected
    Iclient = server.available();   
    if(!Iclient) return ; 
    // Read the first line of the request
    String req = Iclient.readStringUntil('\r'); 
    Iclient.flush();
    // Match the request on GPIO2
    if(req.indexOf("/r20") != -1){                  //<-- http://dario95.ddns.net:28082/r20
      Val2 = 0;
      digitalWrite(2, Val2);                        // Set GPIO2 according to the request
      SendResponse(Val2,2);
      bitClear(MemRele,1);
      MyIftttEvent.setValue(1,Stati[1]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
    else if(req.indexOf("/r21") != -1){             //<-- http://dario95.ddns.net:28082/r21
      Val2 = 1;
      digitalWrite(2, Val2);                        // Set GPIO2 according to the request
      SendResponse(Val2,2);
      bitSet(MemRele,1);
      MyIftttEvent.setValue(1,Stati[2]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
    else if(req.indexOf("/r2?") != -1){              //<-- http://dario95.ddns.net:28082/r2?
      SendResponse(Val2,2);
    }
//----------------------------------------
// Match the request on GPIO1
#ifndef rele1Broken
    else if(req.indexOf("/r10") != -1){             //<-- http://dario95.ddns.net:28082/r10
      Val1 = 0;
      digitalWrite(LED_BUILTIN, Val1);              // Set GPIO1 according to the request
      SendResponse(Val1,1);
      bitClear(MemRele,0);
      MyIftttEvent.setValue(1,Stati[3]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
    else if(req.indexOf("/r11") != -1){             //<-- http://dario95.ddns.net:28082/r11
      Val1 = 1;
      digitalWrite(LED_BUILTIN, Val1);              // Set GPIO1 according to the request
      SendResponse(Val1,1);
      bitSet(MemRele,0);
      MyIftttEvent.setValue(1,Stati[4]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
#endif
#ifdef rele1Broken
    else if(req.indexOf("/r10") != -1){             //<-- http://dario95.ddns.net:28082/r10
      Val1 = 1;
      digitalWrite(LED_BUILTIN, Val1);              // Set GPIO1 according to the request
      SendResponse(Val1,1);
      bitSet(MemRele,0);
      MyIftttEvent.setValue(1,Stati[3]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
    else if(req.indexOf("/r11") != -1){             //<-- http://dario95.ddns.net:28082/r11
      Val1 = 0;
      digitalWrite(LED_BUILTIN, Val1);              // Set GPIO1 according to the request
      SendResponse(Val1,1);
      bitClear(MemRele,0);
      MyIftttEvent.setValue(1,Stati[4]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
 #endif
    else if(req.indexOf("/r1?") != -1){             //<-- http://dario95.ddns.net:28082/r1?
      SendResponse(Val1,1);
    }

//----------------------------------------
// Match the request on both of them
#ifndef rele1Broken
    if(req.indexOf("/rf0") != -1){                  //<-- http://dario95.ddns.net:28082/rf0
      Val2 = 0;
      digitalWrite(2, Val2);                        // Set GPIO2 according to the request
      digitalWrite(LED_BUILTIN, Val2);              // Set GPIO1 according to the request
      SendResponse(Val2,3);
      bitClear(MemRele,0);
      bitClear(MemRele,1);
      MyIftttEvent.setValue(1,Stati[0]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
    else if(req.indexOf("/rf1") != -1){             //<-- http://dario95.ddns.net:28082/rf1
      Val2 = 1;
      digitalWrite(2, Val2);                        // Set GPIO2 according to the request
      digitalWrite(LED_BUILTIN, Val2);              // Set GPIO1 according to the request
      SendResponse(Val2,3);
      bitSet(MemRele,0);
      bitSet(MemRele,1);
      MyIftttEvent.setValue(1,stati[5]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
 #endif
#ifdef rele1Broken
    if(req.indexOf("/rf0") != -1){                  //<-- http://dario95.ddns.net:28082/rf0
      Val2 = 0;
      digitalWrite(2, Val2);                        // Set GPIO2 according to the request
      digitalWrite(LED_BUILTIN, 1);                 // Set GPIO1 according to the request
      SendResponse(Val2,3);
      bitSet(MemRele,0);
      bitClear(MemRele,1);
      MyIftttEvent.setValue(1,Stati[0]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
    else if(req.indexOf("/rf1") != -1){             //<-- http://dario95.ddns.net:28082/rf1
      Val2 = 1;
      digitalWrite(2, Val2);                        // Set GPIO2 according to the request
      digitalWrite(LED_BUILTIN, 0);                 // Set GPIO1 according to the request
      SendResponse(Val2,3);
      bitClear(MemRele,0);
      bitSet(MemRele,1);
      MyIftttEvent.setValue(1,Stati[5]);
      WriteEE(0,MemRele);
      if (MyIftttEvent.connect()) MyIftttEvent.post(); 
    }
 #endif 
    else if(req.indexOf("/rq") != -1){              //<-- http://dario95.ddns.net:28082/rq
      SendStatusRele(digitalRead(LED_BUILTIN),digitalRead(2));
    //SendResponse(digitalRead(LED_BUILTIN),digitalRead(2));
    }
 
//Richiesta reset del modulo
    else if(req.indexOf("/rst") != -1){          
      ESP.restart();     
      return;
    }
  
//Richiesta non riconosciuta  
    else{
      debugln("invalid request");
      Iclient.stop();
      return;
    }
  }

  if(ADAflag == true){
// Ensure the connection to the MQTT server is alive (this will make the first
// connection and automatically reconnect when disconnected).  See the MQTT_connect
// function definition further below.
    MQTT_connect();
    Adafruit_MQTT_Subscribe *subscription;  
    while ((subscription = mqtt.readSubscription(CycleTime/2))) { 
      if (subscription == &ESP01_2LampsTLC) { 
        if (strcmp((char *)ESP01_2LampsTLC.lastread, "ON") == 0){
          digitalWrite(LED_BUILTIN, LOW); 
        }
        if (strcmp((char *)ESP01_2LampsTLC.lastread, "OFF") == 0) {
          digitalWrite(LED_BUILTIN, HIGH); 
        }     
      }
      if (subscription == &ESP01_2LampsLAMP) { 
        if (strcmp((char *)ESP01_2LampsLAMP.lastread, "ON") == 0){
          digitalWrite(2, HIGH); 
        }
        if (strcmp((char *)ESP01_2LampsLAMP.lastread, "OFF") == 0) {
          digitalWrite(2, LOW); 
        }       
      } 
    }
  }
}


//$$$$$$$$$$$$$$$$$$$  S U B R O U T I N E S  $$$$$$$$$$$$$$$$$$$$$$
////////////////////
void SendResponse(uint8_t valore, uint8_t pin){
  Iclient.flush();
// Prepare the response
  String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n<H1>GPIO";
  response += pin;
  response += " is now ";
  response += "<font size=\"8\">";
  response += (valore)?"high":"low";
  response += "</font></html>\n";
// Send the response to the client
  Iclient.print(response);
  delay(1);
  debugln("Client disonnected");
  ESP.wdtFeed();
// The client will actually be disconnected 
// when the function returns and 'client' object is detroyed
}


////////////////////
void SendStatusRele(uint8_t val1, uint8_t val2){
  Iclient.flush();
// Prepare the response
  String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
  response += "TLC:  ";
  #ifdef rele1Broken
  response += (val1)?"off":"ON";
  #endif
  #ifndef rele1Broken
  response += (val1)?"ON":"off";
  #endif
  response +="\n";
  response += "Lamp: ";
  response += (val2)?"ON":"off";
  response += "</html>\n";
// Send the response to the client
  Iclient.print(response);
  delay(1);
  debugln("Client disonnected");
  ESP.wdtFeed();
// The client will actually be disconnected 
// when the function returns and 'client' object is detroyed
}


//////////////////http://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html#eeprom
void WriteEE(int loc,int value){
//replace values in byte-array cache with modified data 
//no changes made to flash, all in local byte-array cache
  EEPROM.put(loc,value);
// actually write the content of byte-array cache to hardware flash.
// flash write occurs if and only if one or more byte
// in byte-array cache has been changed, but if so, ALL EEsize bytes are 
// written to flash
  EEPROM.commit();  
}


//////////////////
void MQTT_connect(){
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) return;
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) return;
  }
}


//////////////////
void debug(String message){
#ifdef SERIAL_DEBUG
  Serial.print(message);
#endif
}
void debugc(char message){
#ifdef SERIAL_DEBUG
  Serial.print(message);
#endif
}
void debugi(int message){
#ifdef SERIAL_DEBUG
  Serial.print(message);
#endif
}
void debugln(String message){
#ifdef SERIAL_DEBUG
  Serial.println(message);
#endif
}
void debugln(){
#ifdef SERIAL_DEBUG
  Serial.println();
#endif
}
