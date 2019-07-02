#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WiFiManagerByjulfi.h>
#include <ArduinoJson.h>
#include <NTPClientByjulfi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>

/*--------------------------------------------------------------------------------
Time constants for time management
--------------------------------------------------------------------------------*/
#define MSECOND  1000
#define MMINUTE  60*MSECOND
#define MHOUR    60*MMINUTE
#define MDAY 24*MHOUR

/*--------------------------------------------------------------------------------
Pins number for the green, red and blue's MOFETS
--------------------------------------------------------------------------------*/
#define REDPIN 13
#define GREENPIN 12
#define BLUEPIN 14

int dataSmartEcl[3]; //Array that contains the 3 UNIX timestamp used in the smart light mode.
String couleurComp[2] = {"255,72,0","255,255,255"};


unsigned long utcOffsetInSeconds = 7200; 

/*--------------------------------------------------------------------------------
Variable that define the operating mode and the refresh time
--------------------------------------------------------------------------------*/
unsigned long refresh = 10000;

String mode = "Desative"; //The operating mode of smarth light is Disabled by default

uint8_t go1 = 0; //True if we have to delete some data stored in the memory.

uint8_t go2 = 0;   


//Used in the loop function to watch and react depending on the user input
unsigned long previousLoopMillis = 0; 
unsigned long previousAlarme = 0;

/*--------------------------------------------------------------------------------
All the alarm related variables
--------------------------------------------------------------------------------*/
bool wakeHour = false; //True if we active the alarm mode
int WakeTime; //Contain the waking hour in unix timestamp
int refreshAlarme; //Contains the refresh rate
int ecartFromHigh; //Contains the fixed gap from high

//The values between 0 and 255 for green, red and blue
uint8_t AlarmeRed = 0;
uint8_t AlarmeGreen = 0;
uint8_t AlarmeBlue = 0;

/*--------------------------------------------------------------------------------
Objects declaration
--------------------------------------------------------------------------------*/

ESP8266WebServer server(80); //Webserver
WebSocketsServer webSocket = WebSocketsServer(81); //Websocket

WiFiUDP ntpUDP; //UDP communication for NTPClient
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org",utcOffsetInSeconds);  //NTP server communication

HTTPClient http; //HTTP to emit request


/*--------------------------------------------------------------------------------
OTA function
--------------------------------------------------------------------------------*/
void confOTA() {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("ESP8266_LEDstrip");

  ArduinoOTA.onStart([]() {
    Serial.println("/!\\ Maj OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n/!\\ MaJ terminee");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progression: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

String split(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/*--------------------------------------------------------------------------------
Set the loop refresh rate acording to the gap between the actual unix timestamp
and the alarm set by the user.
--------------------------------------------------------------------------------*/
void Alarme(){

  //Unix actual timestamp is less than 5 minutes lower than time set.
  if((timeClient.getEpochTime() < (WakeTime - 300)) && refreshAlarme != 60000){
    refreshAlarme = 60000; //Refresh rate

    Serial.println(timeClient.getEpochTime());
    Serial.println("<");
    Serial.println(WakeTime);
    Serial.println(refreshAlarme);

    //Shutdown LED
    analogWrite(REDPIN, 0);
    analogWrite(GREENPIN, 0);
    analogWrite(BLUEPIN, 0);

  //Actual unix timestamp higher than time set
  }else if(timeClient.getEpochTime() >= WakeTime){
    WakeTime += 86400; //Add 1 day to the original waketime
    refreshAlarme = 3600000; //Next refresh in 1 hour

  //Unix actual timestamp is more than 5 minutes lower than time set.
  }else if(timeClient.getEpochTime() > (WakeTime - 300)){
    int time = timeClient.getEpochTime();

    //First iteration change
    if(refreshAlarme != 1000){
      Serial.println(time);
      Serial.println(WakeTime);
      Serial.println("refresh toute les secondes");

      refreshAlarme = 1000; //Refresh every seconds
      ecartFromHigh = WakeTime - time; //maximum gap between waktetime and actual time
    }
    
    //Define color thanks to a mapping between 0 and the color and thanks to the gap between waketime and time set.
    int red = map((WakeTime - time), ecartFromHigh, 0, 0, AlarmeRed);
    int green = map((WakeTime - time), ecartFromHigh, 0, 0, AlarmeGreen);
    int blue = map((WakeTime - time), ecartFromHigh, 0, 0, AlarmeBlue);

    Serial.println(red);
    Serial.println(green);
    Serial.println(blue);

    //Turn on led in the colors set above
    analogWrite(REDPIN, red);
    analogWrite(GREENPIN, green);
    analogWrite(BLUEPIN, blue);

  }
}

/*--------------------------------------------------------------------------------
Return if we are near the sunset or sunrise
--------------------------------------------------------------------------------*/
String sunPosition(int dataSmartEcl[], uint8_t longueur){
  
  //compare unix actual timestamp with the sunset and sunrise timestamp
  //Then detect if we are near the sunet or the sunrise and return the correct indaction.
  if(abs(dataSmartEcl[2] - dataSmartEcl[0]) > abs(dataSmartEcl[2] - dataSmartEcl[1])){
    return "soir";
  }else{
    return "matin";
  }
}

/*--------------------------------------------------------------------------------
Return the gap between day and night
--------------------------------------------------------------------------------*/
uint16_t checkTime(int dataSmartEcl[], uint8_t longueur){
  uint8_t index;
  //According to the value return we compare the actual unix timestamp to the sunset/sunrise
  if(sunPosition(dataSmartEcl, 3) == "matin"){index = 0;}
  else{index=1;}

  int ecart = dataSmartEcl[2] - dataSmartEcl[index];
  if(ecart < 0 && ecart > -3600){ //If the gap in the slice that we defined as near the sunset/sunrise
    int result = map(ecart, -3600, 0, 0, 3600); //It return a positive result
    return result;
  }else if(ecart < -3600){return 0;} //it return 0 if we are not near that we de define as near the sunet/sunrise
  else if(ecart > 0){return 3600;} //else in the case that we are higher we return
  else{return 0;} //it return 0

}

/*--------------------------------------------------------------------------------
Return values red, green and blue in function of the gap between day and night
--------------------------------------------------------------------------------*/
void displayColors(int dataSmartEcl[],uint8_t longueur, int cooldown, String mode){
  /*--------------------------------------------------------------------------------
  According to the solar poisition it display the color that the user select
  --------------------------------------------------------------------------------*/
  Serial.println(sunPosition(dataSmartEcl, 3));
  if((sunPosition(dataSmartEcl, 3) == "soir") && (mode = "SmartLight")){ //If we are in the evening slice
    Serial.println("soir");

    //Mapping the color displayed according to the unix timestamp position in the slice of time defined.
    uint8_t rouge = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[1],',',0)).toInt(),(split(couleurComp[0],',',0)).toInt());
    uint8_t vert = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[1],',',1)).toInt(),(split(couleurComp[0],',',1)).toInt());
    uint8_t bleu = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[1],',',2)).toInt(),(split(couleurComp[0],',',2)).toInt());

    analogWrite(REDPIN, rouge);
    analogWrite(GREENPIN, vert);
    analogWrite(BLUEPIN, bleu);
    
    //sending the rgb value though the websocket.
    String rgb = ("rgb("+String(rouge, DEC) +","+String(vert,DEC)+","+String(bleu,DEC)+")");
    Serial.println(rgb);
    webSocket.sendTXT(0,rgb);

  }else{
    Serial.println("journee");

    //Mapping the color displayed according to the unix timestamp position in the slice of time defined.
    uint8_t rouge = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[0],',',0)).toInt(),(split(couleurComp[1],',',0)).toInt());
    uint8_t vert = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[0],',',1)).toInt(),(split(couleurComp[1],',',1)).toInt());
    uint8_t bleu = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[0],',',2)).toInt(),(split(couleurComp[1],',',2)).toInt());

    analogWrite(REDPIN, rouge);
    analogWrite(GREENPIN, vert);
    analogWrite(BLUEPIN, bleu);

    //sending the rgb value though the websocket.
    String rgb = ("rgb("+String(rouge, DEC) +","+String(vert,DEC)+","+String(bleu,DEC)+")");
    Serial.println(rgb);
    webSocket.sendTXT(0,rgb);
  }

}

/*--------------------------------------------------------------------------------
Return sunrise and sunsert unix timestamp
--------------------------------------------------------------------------------*/
int requete(String Apikey, String ville,String type){
  String requeteHTTP =("http://api.openweathermap.org/data/2.5/weather?q=" + ville + "&APPID="
  + Apikey); 
  int timestamp;

  http.begin(requeteHTTP);  //Specify request destination
  int httpCode = http.GET();

  if (httpCode > 0) { //Check the returning code
 
  String payload = http.getString();   //Get the request response payload

  DynamicJsonDocument doc(765);
  DeserializationError error = deserializeJson(doc, payload);
  if(type == "sunrise"){
    timestamp = doc["sys"]["sunrise"];
  }else if(type == "sunset"){
    timestamp = doc["sys"]["sunset"];
  }
  

  }else{
    timestamp = 0;
  }
  http.end();   //Close connection
  return timestamp;
}

/*--------------------------------------------------------------------------------
Return a selection of data in excess saved in the memory
--------------------------------------------------------------------------------*/
void suprdata(String data[55], int tabIndex[5], String nomFichier, int limiteSave,int beginSave){
  File ftemp = SPIFFS.open(nomFichier, "w"); //Open save.csv in writing mode
  
  for(uint8_t x=beginSave;x<limiteSave;x++){ //For each colors saved
    char msg[60];
    int index = tabIndex[x]; //index equal the position of each elements
    String ligne = data[index]; //ligne equal to the color
    sprintf(msg, "%s;",ligne.c_str()); //we appends this data under variable msg

    if (nomFichier == "/saveS.csv"){
      Serial.println(msg);
      couleurComp[x-beginSave] = msg;

    }
    ftemp.print(msg);

    
  }
  ftemp.close();
}


/*--------------------------------------------------------------------------------
Check if we need to delete some data
--------------------------------------------------------------------------------*/
bool checkSpace(uint8_t count, uint8_t longueur, String go){
  if(go == "go1"){
    Serial.println("longeur = 5");
    go1 += count;
    if(go1 >= longueur){
      return 1;
    }else{
      return 0;
    }
  }else{
    go2 += count;
    if(go2 >= longueur){
      return 1;
    }else{
      return 0;
    }
  }
}

/*--------------------------------------------------------------------------------
Delete data selected to be delete
--------------------------------------------------------------------------------*/
void suprSelect(String nom, String nomFichier, uint8_t nombre, uint8_t Nsuppr){
  File file = SPIFFS.open(nomFichier, "r");
 
  String save[55] = {};
  int index[5] = {};
  int longueur=1;


  for(uint8_t i=0;i<nombre;i++){ //Loop this 5 times
    String part = file.readStringUntil(';'); //Read the lign until caracter ';'
    Serial.println(part);
    if(longueur > Nsuppr){
      save[longueur] = part; //Add in the array the saved color
      index[i] = longueur; //Also add in another array the length
    }

    longueur += part.length();
  }
  if(nom == "n0"){
    go1 --;

    file.close(); 
    suprdata(save, index, "/save.csv",5,1);
  }else if(nom == "n1"){
    go2 -=2;

    file.close(); 
    suprdata(save, index, "/saveS.csv",4,2);
  }
}

/*--------------------------------------------------------------------------------
Add data in the chip memory
--------------------------------------------------------------------------------*/
void addData(uint16_t couleur, uint8_t * couleurSave, String nomFichier, uint8_t index){
  uint16_t save = (uint16_t) strtol((const char *) &couleurSave[index], NULL, 10);
  //If we want to add alarm data we juste need to erase the old ones
  //Else we simply add
  File f = SPIFFS.open(nomFichier, "a+");
  if (nomFichier == "/saveA.csv" && couleur == 'R'){
    File f = SPIFFS.open(nomFichier, "w");
  }

  if (!f) {
    Serial.println("erreur ouverture fichier!");
  }else{
    if(couleur == 'R'){
        char buffred[16];
        sprintf(buffred,"%d,",save);
        Serial.println(buffred);
        f.print(buffred);
        if(nomFichier == "/saveA.csv"){ AlarmeRed = save;}

    }else if(couleur == 'G'){
        char buffgreen[16];
        sprintf(buffgreen,"%d,",save);
        f.print(buffgreen);
        Serial.println(buffgreen);
        if(nomFichier == "/saveA.csv"){AlarmeGreen = save;}
        
        
    }else if(couleur == 'B'){
        char buffblue[16];
        sprintf(buffblue,"%d;",save);
        Serial.println(buffblue);
        f.print(buffblue);
        if(nomFichier == "/saveA.csv"){ AlarmeBlue = save;}

        //Check if the colors saved is not higher than 5
       if(nomFichier != "/saveA.csv"){
          if(index == 3 && checkSpace(1,2,"go2")){
            Serial.println("Suppression save1");
            suprSelect("n1","/saveS.csv",5,12);
            //If it the case we liberating memory

          }else if(index == 2 && checkSpace(1,5,"go1")){
            Serial.println("Suppression save");
            suprSelect("n0","/save.csv",5,1);

          }
       }
    }
    f.close();
  }
}

/*--------------------------------------------------------------------------------
Define if the wakeup time is for the current or the followed day
--------------------------------------------------------------------------------*/
int WakeUPday(int heureWake, int MinWake){
  Serial.println(heureWake);
  Serial.println(timeClient.getHours());
  if(heureWake < timeClient.getHours()){
    return timeClient.getRealDay()+1;
  }else if(heureWake == timeClient.getHours() && MinWake < timeClient.getMinutes()){
    return timeClient.getRealDay()+1;
  }else{
    return timeClient.getRealDay();
    Serial.println(timeClient.getRealDay());
  }
}

/*--------------------------------------------------------------------------------
Treat websocket requests coming from the webserver
--------------------------------------------------------------------------------*/
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type == WStype_TEXT){
    uint16_t couleur = (uint16_t) strtol((const char *) &payload[1], NULL, 10);
    if(payload[0] == 'A'){
      if(payload[1] == 'O'){
        wakeHour = true;
      }else if(payload[1] == 'F'){
        wakeHour = false;
      }else{
        int heure = 0;
        int minute = 0;

        if(couleur < 1000){
          String str = String(couleur, DEC);
          heure = (str.substring(0,1)).toInt();
          minute = (str.substring(1,3)).toInt();
        }else{
          String str = String(couleur, DEC);
          Serial.println((str.substring(0,1)).toInt());
          Serial.println((str.substring(1,3)).toInt());
          heure = (str.substring(0,2)).toInt();
          minute = (str.substring(2,4)).toInt();
        }

        struct tm info;
        char buffer[80];

        info.tm_min = minute;
        info.tm_hour = heure;
        info.tm_year =timeClient.getYear() - 1900;
        info.tm_mon = timeClient.getMonth()-1;
        info.tm_mday = WakeUPday(heure, minute);
        info.tm_sec = 1;
        info.tm_isdst = -1;

        WakeTime = mktime(&info);

        Serial.println(mktime(&info));

        Alarme();
      }
    }
    if(payload[0] == 'R'){
      analogWrite(REDPIN, couleur);

      Serial.print("rouge = ");
      Serial.println(couleur);
    }
    if(payload[0] == 'G'){
      analogWrite(GREENPIN, couleur);

      Serial.print("vert = ");
      Serial.println(couleur);
    }
    if(payload[0] == 'B'){
      analogWrite(BLUEPIN, couleur);

      Serial.print("bleu = ");
      Serial.println(couleur);
    }
    if(payload[0] =='s'){
      if(payload[1] == 'T'){
        addData(payload[2],payload,"/saveS.csv",3);
      }else if(payload[1] == 'A'){
        Serial.println("ok");
        
        addData(payload[2],payload,"/saveA.csv",3);
      }else{
        addData(payload[1],payload,"/save.csv",2);
      }
    }
    if(payload[0] == '#'){
      if(payload[1] == '1'){
        dataSmartEcl[0] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunrise")+utcOffsetInSeconds);
        dataSmartEcl[1] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunset")+utcOffsetInSeconds);
        dataSmartEcl[2] = (timeClient.getEpochTime());

        if (checkTime(dataSmartEcl, 3)==0){
          refresh = 10*MMINUTE;
          mode="Active";
          Serial.println("mode = active");
          displayColors(dataSmartEcl,3,3600,"SmartLight");
        }else if(checkTime(dataSmartEcl, 3)>=0){
          refresh = 5*MMINUTE;
          mode="Process";
          Serial.println("mode = process");
          displayColors(dataSmartEcl,3,3600,"SmartLight");
        }
      
      }else if(payload[1] == '2'){
        mode="Desative";
        Serial.println("mode = desactive");
      }
    }
  }

}

void setup() {

  Serial.begin(115200);

  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);

    //WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect("Webportail");
  Serial.println("connected...yeey :)");

  confOTA();

   if(!SPIFFS.begin()) {
  Serial.println("Erreur initialisation SPIFFS");
  }



  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/style", SPIFFS, "/style");
  server.serveStatic("/save.csv", SPIFFS, "/save.csv");
  server.serveStatic("/saveA.csv", SPIFFS, "/saveA.csv");
  server.serveStatic("/saveS.csv", SPIFFS, "/saveS.csv");
  server.serveStatic("/iconWeldy.ico", SPIFFS, "/iconWeldy.ico");
  server.serveStatic("/alarm.html", SPIFFS, "/alarm.html");

  
  timeClient.begin();
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent); //websocket callback function
}

        
void loop() {

  /*--------------------------------------------------------------------------------
  Iterative tests to update led status according to the smart light
  --------------------------------------------------------------------------------*/
  unsigned long currentLoopMillis = millis();
  if(currentLoopMillis - previousLoopMillis >= refresh){
    
    if(((timeClient.getFormattedTime()).substring(0,2)).toInt() < 1){
      if(((timeClient.getFormattedTime()).substring(3,6)).toInt() < 30){
        dataSmartEcl[0] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunrise")+utcOffsetInSeconds);
        dataSmartEcl[1] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunset")+utcOffsetInSeconds);
      }
    }
    dataSmartEcl[2] = (timeClient.getEpochTime());
    if(mode=="Active" or mode=="Process"){
      displayColors(dataSmartEcl,3,3600,"SmartLight");
    }
    previousLoopMillis = millis();
  }


  /*--------------------------------------------------------------------------------
  Iterative alarm test
  --------------------------------------------------------------------------------*/
  if(wakeHour){
    unsigned long currentAlarmeMillis = millis();
    if(currentAlarmeMillis - previousAlarme >= refreshAlarme){
      Alarme();
      previousAlarme = millis();
    }
  }
  /*--------------------------------------------------------------------------------
  Watchdogs!
  --------------------------------------------------------------------------------*/
  timeClient.update();
  ArduinoOTA.handle(); //OTA function
  webSocket.loop(); //websocket function
  server.handleClient(); //webserver function
}
