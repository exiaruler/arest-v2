/*
  This a simple example of the aREST Library for the ESP8266 WiFi chip.
  See the README file for more details.
  Written in 2015 by Marco Schwartz under a GPL license.
*/

// Import required libraries
#include <ESP8266WiFi.h>
#include <aREST.h>
#include "Servo.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <AFArray.h>
#include <RGBLed.h>


// Create aREST instance
aREST rest = aREST();

// WiFi parameters
const char* ssid = "veda";
const char* password = "gn002dynames";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80
// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Variables to be exposed to the API
// global variable
String ok="OK";
// global operations
boolean uploadMode;
String device="";
String status="";
String state="";
String warning="";
String devices="";
String queryData="";
boolean backgroundRunning;

// global methods 
String servoMove(Servo servo,int pin,int start,int move,int gap,int between,int loop){
  
  if(gap<0){
    gap=1000;
  }
  if(between<0){
    between=5000;
  }
  if(loop<=0){
    return "loop must be bigger than 0";
  }
  if(start<0){
    return "start position missing";
  }
  servo.attach(pin);
  servo.attach(pin,500,2400);
  if(!servo.attached()){
    return "Servo pin does not exist";
  }
  servo.write(start);
  delay(gap);
  for(int i=0; i<loop; i++){
  //servo.write(start);
  //delay(gap);
  servo.write(move);
  delay(between);
  servo.write(start);
  delay(gap);
  }
  servo.detach();
  return ok;
}
void setColour(int R,int pinR, int G,int pinG, int B,int pinB) {
  analogWrite(pinR,R);
  analogWrite(pinG,G);
  analogWrite(pinB,B);
}
void setColourCommonAn(int R,int pinR, int G,int pinG, int B,int pinB,int postivePin,int current=255){
  analogWrite(postivePin,255);
  RGBLed led(pinR,pinG,pinB, RGBLed::COMMON_ANODE);
  led.setColor(R, G, B);
}
// background task to run
struct backgroundTask{
  String device="";
  String method="";
  int pin=0;
  boolean rgb=false;
  long interval=100;
  // rgb pins
  int rgbRed=0;
  int rgbGreen=0;
  int rgbBlue=0;
  int rgbSet[3];
  int runTarget;
  int count=0;
  int x=1;
  unsigned long d1;
  unsigned long d2;
  //variables to hold our color intensities and direction
  //and define some initial "random" values to seed it
  int red=254;
  int green=1;
  int blue=127;
  int red_direction= -1;
  int green_direction= 1;
  int blue_direction= -1;

};
AFArray<backgroundTask> queue;
backgroundTask task={};
// add task
int addTask(String device,String method,int pin,boolean rgb,int rgbPins [],int interval){
  int index=0;
  boolean exist=false;
  for(int i=0; i<queue.size(); i++){
    backgroundTask t=queue[i];
    if(t.device==device&&t.method==method&&t.interval==interval&&t.pin==pin&&t.rgb==rgb&&t.rgbRed==rgbPins[0]&&t.rgbGreen==rgbPins[1]&&t.rgbBlue==rgbPins[2]){
      exist=true;
      break;
    }
  }
  if(!exist){
    backgroundTask task={device,method,pin,rgb,interval,rgbPins[0],rgbPins[1],rgbPins[2]};
    queue.add(task);
    index=queue.size()-1;
  }
  return index;
}
// for command base
int addTaskManual(String device,String method,int pin,boolean rgb,int rgbR,int rgbB,int rgbG,int interval){
  int index=0;
  return index;
}
// remove device from queue
AFArray<backgroundTask> removeDeviceTask(String device){
  AFArray<backgroundTask> newQueue;
  AFArray<backgroundTask> deviceTasks;
  for(int i=0; i<queue.size(); i++){
    backgroundTask t=queue[i];
    if(t.device!=device){
      newQueue.add(t);
    }else{
      deviceTasks.add(t);
    }
  }
  queue=newQueue;
  return deviceTasks;
}
// add old task back into queue if it does not match with new task added
void addDevicetaskBackToQueue(String method){

}
void ledBlinkBackGround(int pin,long interval){
  task.d1,task.d2;
  task.d2=millis();
  if ( task.d2-task.d1 >= interval){
    task.x=1-task.x;
    task.d1=millis();
    digitalWrite(pin,task.x);
  }
   task.d1,task.d2;
}
void ledBlinkBackGroundRgb(int pin,long interval,int powerPin){
  task.d1,task.d2;
  task.d2=millis();
  if ( task.d2-task.d1 >= interval){
    task.x=1-task.x;
    task.d1=millis();
    //digitalWrite(pin,task.x);
    digitalWrite(powerPin,task.x);
    
  }
   task.d1,task.d2;
}
void rgbFade(int interval){
  task.d1,task.d2;
  task.d2=millis();
  if ( task.d2-task.d1 >= interval){
    task.x=1-task.x;
    task.d1=millis();
    RGBLed led(task.rgbRed,task.rgbGreen,task.rgbBlue, RGBLed::COMMON_ANODE);
    analogWrite(task.pin,255);
    led.fadeIn(255, 0, 0, 5, 100);
    led.fadeOut(255, 0, 0, 5, 100);
  }
   task.d1,task.d2;
}
void rgbCycle(int interval){
  task.d1,task.d2;
  task.d2=millis();
  if ( task.d2-task.d1 >= interval){
    task.x=1-task.x;
    task.d1=millis();
    RGBLed led(task.rgbRed,task.rgbGreen,task.rgbBlue, RGBLed::COMMON_ANODE);
    analogWrite(task.pin,255);

    task.red = task.red + task.red_direction;   //changing values of LEDs
    task.green = task.green + task.green_direction;
    task.blue = task.blue + task.blue_direction;
    //now change direction for each color if it reaches 255
    if (task.red >= 255 || task.red <= 0)
    {
      task.red_direction = task.red_direction * -1;
    }
    if (task.green >= 255 || task.green <= 0)
    {
      task.green_direction = task.green_direction * -1;
    }
    if (task.blue >= 255 || task.blue <= 0)
    {
      task.blue_direction = task.blue_direction * -1;
    }
    led.setColor(task.red,task.green,task.blue);
  }
  task.d1,task.d2;
}

// led blink function
int x = 1;
unsigned long d1, d2;
void ledBlink(int pin,long interval){
  d1,d2;
  pinMode(pin, OUTPUT);
  d2=millis();
  if (d2-d1 >= interval){
    x=1-x;
    d1=millis();
    digitalWrite(pin,x);
  }
  d1,d2;
}
// calculate time for mins
int calculateTime(int t){
 int min=1000*60;
 return min*t;
}

// Declare functions to be exposed to the API
// Setting Routes
int uploadModeConfig(String command){
  int r=0;
  if(command=="true"){
    uploadMode=true;
    r=1;
  }
  if(command=="false"){
    uploadMode=false;
    r=1;
  }
  return r;
}
int backgroundrun(String command){
  backgroundRunning=true;
  return 1;
}

// data structure 
struct route{
  String routeName;
  boolean modes=false;
  String params[15];
};

struct component{
  String part;
  // normal pin
  int pin=0;
  boolean rgb=false;
  boolean main=false;
  // rgb pins red,green and blue
  int rgbPins[3];
  boolean motor=false;
  boolean pinmodeOut=false;
  boolean pinmodeIn=false;
  //Servo servo;
  boolean active=false;
  String status="";
};
struct deviceS{
  String name="";
  String state="";
  String warning="";
  String routes="";
  String status="";
  component components[10];
  String type="";
  String subType="";
  route routesArr[10];
};

// current device which is set when setting modes
deviceS deviceSet={};
int deviceArrIndex=0;
// check route can be use for that device. validate command for that route if there multiple modes
boolean validateRoute(String routeS,String param=""){
  boolean res=false;
  int length = sizeof(deviceSet.routesArr) / sizeof(deviceSet.routesArr[0]);
  for(int i=0; i<length; i++){
    route rou=deviceSet.routesArr[i];
    if(routeS.equals(rou.routeName)){
      // if there modes loop through to validate
      if(rou.modes){
        boolean end=false;
        int modesLen=sizeof(rou.params) / sizeof(rou.params[0]);
        for(int a=0; a<modesLen; a++){
          String mode=rou.params[a];
          if(param==mode){
            break;
          }
          if(mode==""){
            end =true;
            break;
          }
        }
        if(end){
          warning="invalid param";
          deviceSet.warning=warning;
          break;
        }
      }

      res=true;
      break;
    }
    if(rou.routeName.equals("")){
      break;
    }
  }
  return res;
}
// devices
int ledPin=D2;
String commandSep="|~||";
deviceS deviceArr[]={{
  "Aerial","","","","",{{"Main",D4,false,true,{},false,true},{"Shelll Unit",D8,true,false,{D1,D2,D3},false,true}},"WFM","Gundam",
  {{"randPermet",false},
  {"setPermet",true,{"0","1","2","3","4","5","6","7","8","9","10"}},
  {"setMain",true,{"On","Off"}}}
},
{"Build Strike EG","","","",""}};

int setMain(String command){
  int r=0;
  warning="";
  if(validateRoute("setMain",command)){
    
  }else{
    deviceSet.warning=="device does not have this function";
  }
  return r;
}
// customRGB
void customRGB(String param){
  
}
// aerial route
int randPermet(String command){
  warning="";
  int r=0;
  if(validateRoute("randPermet")){
    if(turnOnShellRandom()){
      int score=permetScoreRandom();
      if(deviceSet.status!=""&&deviceSet.status!="off"){
        score=random(deviceSet.status.toInt(),11);
      }
      deviceSet.status=setPermetScore(score);
    }else{
      setPermetScore(0);
      deviceSet.status="off";
    }
    r=1;
  }else{
    deviceSet.warning="device does not have this function";
  }
  
  deviceSet.warning=warning;
  return r;
}
// aerial route
int setPermet(String score){
  warning="";
  int r=0;
  if(validateRoute("setPermet",score)){
    if(score!=""){
      int convertedScore=score.toInt();
      String setPermet=setPermetScore(convertedScore);
      deviceSet.status=setPermet;
      deviceSet.warning=warning;
      r=1;
    }else{
      warning="score entered not valid";
      deviceSet.warning=warning;
    }
  }else{
    //warning="device does not have this function";
  }
  return r;
}

boolean turnOnShellRandom(){
  boolean res=false;
  int ranDom;
  ranDom = random(1,3);
  if(ranDom==2){
    res=true;
  }
  return res;
}
int permetScoreRandom(){
  int score;
  score=random(1,11);
  return score;
}
String setPermetScore(int permet){
  String score="";
  // get component that has rgb pins
  int length = sizeof(deviceSet.components) / sizeof(deviceSet.components[0]);
  // clear any exist task
  if(queue.size()>0){
    removeDeviceTask(deviceSet.name);
    Serial.println("tasks clear");
  }
  for(int i=0; i<length; i++){
    component comp=deviceSet.components[i];
    if(comp.rgb){
      int rPin=comp.rgbPins[0],gPin=comp.rgbPins[1],bPin=comp.rgbPins[2];
      
      if(permet==0){
        analogWrite(comp.pin,0);
        setColour(255,rPin,255,gPin,255,bPin);
        score="off";
        comp.active=false;
        comp.status="";
      }
      if(permet==1){
        score="1";
        int pins []={rPin,0,0};
        setColour(100,rPin,255,gPin,255,bPin);
        addTask(deviceSet.name,"ledBlinkBackGround",comp.pin,false,pins,3000);
        comp.active=true;
        comp.status=permet;
      }
      if(permet==2){
        analogWrite(comp.pin,255);
        setColour(200,rPin,255,gPin,255,bPin);
        score="2";
        comp.active=true;
        comp.status=permet;
      }
      if(permet>=3&&permet<=5){
        analogWrite(comp.pin,255);
        score="";
        setColour(0,rPin,255,gPin,255,bPin);
        score=permet;
        comp.active=true;
        comp.status=permet;
      }
      // blue
      if(permet>=6&&permet<8){
        score="";
        score=permet;
        comp.active=true;
        comp.status=permet;
        setColourCommonAn(0,rPin,0,gPin,255,bPin,comp.pin);
      }
      // white
      if(permet==8){
        score="";
        score=permet;
        comp.active=true;
        comp.status=permet;
        setColourCommonAn(255,rPin,255,gPin,255,bPin,comp.pin);
      }
      // go through all the colours
      if(permet>=9){
        score="";
        comp.active=true;
        comp.status=permet;
        score=permet;
        int pins []={rPin,gPin,bPin};
        addTask(deviceSet.name,"rgbCycle",comp.pin,true,pins,5);
      }

      
    }
    deviceSet.components[i]=comp;
  }
  
  return score;
}
// write route string
String writeRoutesString(){
  String paramSep="|~||";
  String routes=paramSep;
  int length = sizeof(deviceSet.routesArr) / sizeof(deviceSet.routesArr[0]);
  for(int i=0; i<length; i++){
    route rou=deviceSet.routesArr[i];
    if(rou.routeName!=""){
      if(routes.equals(paramSep)){
        routes=routes+rou.routeName;
      }else{
        routes=routes+"|"+rou.routeName;
      }
      if(rou.modes){
        String modes="(";
        int paramLen=sizeof(rou.params) / sizeof(rou.params[0]);
        for(int a=0; a<paramLen; a++){
          String m=rou.params[a];
          if(m!=""){
            if(modes.equals("(")){
              modes=modes+m;
            }else{
              modes=modes+"~"+m;
            }
          }else{
            break;
          }
        }
        modes=modes+")";
        routes=routes+modes;
      }
    }else{
      break;
    }
  }
  
  return routes;
}
// return method and param
String * returnMethodandParam(String method){
  static String arr[2];
  int indexA=method.indexOf(" ");
  int indexB=method.length();
  String m=method.substring(0,indexA);
  Serial.println(m);
  String p=method.substring(indexA+1,indexB);
  Serial.println(p);
  arr[0]=m;
  arr[1]=p;
  return arr;
}
// return boolean from string
boolean stringToBool(String boo){
  boolean res=false;
  if(boo.startsWith("true")){
    res=true;
  }
  return res;
}
// return pin from string
uint8_t stringToPinIntDig(String pin){
  uint8_t pinR=D0;
  //Serial.println(pin);
  if(pin.startsWith("D1")){
    pinR=D1;
  }
  if(pin.startsWith("D2")){
    pinR=D2;
  }
  if(pin.startsWith("D3")){
    pinR=D3;
  }
  if(pin.startsWith("D4")){
    pinR=D4;
  }
  if(pin.startsWith("D5")){
    pinR=D5;
  }
  if(pin.startsWith("D6")){
    pinR=D6;
  }
  if(pin.startsWith("D7")){
    pinR=D7;
  }
  if(pin.startsWith("D8")){
    pinR=D8;
  }
  if(pin.startsWith("D9")){
    pinR=D9;
  }
  if(pin.startsWith("D10")){
    pinR=D10;
  }
  Serial.println(pinR);
  return pinR;
}
// return each param variable in array
AFArray<String> parameterArray(int paramsSize,String param){
  AFArray<String> arr;
  String split="";
  
  for(int i=0; i<paramsSize; i++){
    if(split==""){
      int spaceIndex=param.indexOf(" ");
      String variable=param.substring(0,spaceIndex+1);
      Serial.println(variable);
      arr.add(variable);
      split=param.substring(spaceIndex+1,param.length());
    }
    else if(split!=""){
      int spaceIndex=split.indexOf(" ");
      String variable=split.substring(0,spaceIndex+1);
      Serial.println(variable);
      arr.add(variable);
      split=split.substring(spaceIndex+1,split.length());
    }
    
 }
  
  return arr;
}
// case of methods
int methodCheck(String method,String param){
  int r=0;
  if(method=="setColourCommonAn"){
    AFArray<String> paramArr=parameterArray(7,param);
    uint8_t r=stringToPinIntDig(paramArr[1]);
    uint8_t g=stringToPinIntDig(paramArr[3]);
    uint8_t b=stringToPinIntDig(paramArr[5]);
    uint8_t postive=stringToPinIntDig(paramArr[6]);
    r=1;
    setColourCommonAn(paramArr[0].toInt(),r,paramArr[2].toInt(),g,paramArr[4].toInt(),b,postive,255);
    
  }
  // move servo custom
  if(method=="servoMove"){
    // get device servo
    // get parameters and excute servo method
    r=1;
  }
  return r;
}
// custom commands 
int command(String input){
  int r=0;
  if(input!="1"){
    Serial.println(input);
    String* arr=returnMethodandParam(input);
    String method=arr[0];
    String para=arr[1];
    r=methodCheck(method,para);
  }
  return r;
}

// set device to adjust settings
int changeDevice(String command){
  int r=0;
  int length = sizeof(deviceArr) / sizeof(deviceArr[0]);
  if(deviceSet.name!=command){
    // update device in array
    deviceArr[deviceArrIndex]=deviceSet;
    for(int i=0; i<length; i++){
      String d=deviceArr[i].name;
      if(d==command){
        // set index of the device in use
        deviceSet=deviceArr[i];
        deviceArrIndex=i;
        setRestDeviceVariables();
        queryData="";
        r=1;
        break;
      }
    }
  }
  return r;
}
// query data
int getDeviceData(String command){
  int r=0;
  // show all query commands
  if(command.equals("1")){
    queryData="routes,type,subtype,components,background";
    r=1;
  }
  if(command.equals("routes")){
    if(deviceSet.routes.equals("")){
      deviceSet.routes=writeRoutesString();
    }
    queryData=deviceSet.routes;
    r=1;
  }
  if(command.equals("type")){
    queryData=deviceSet.type;
    r=1;
  }
  if(command.equals("subtype")){
    queryData=deviceSet.subType;
    r=1;
    
  }
  if(command.equals("components")){
    String array="[";
    int length = sizeof(deviceSet.components) / sizeof(deviceSet.components[0]);
    for(int i=0; i<length; i++){
      component comp=deviceSet.components[i];
      if(comp.part!=""){
        String json="{";
        json=json+"part:"+comp.part+",";
        json=json+"active:"+comp.active+",";
        json=json+"status:"+comp.status+",";
        json=json+"main:"+comp.main;
        json=json+"}";
        if(array.equals("[")){
          array=array+json;
        }else{
          array=array+","+json;
        }
      }
    }
    array=array+"]";
    queryData=array;
    r=1;
  }
  // make array of background tasks running 
  if(command.equals("background")){
    String array="[";
    if(queue.size()>0){
      for(int i=0; i<queue.size(); i++){
        backgroundTask t=queue[i];
          if(t.device!=""){
            String json="{";
            json=json+"device:"+t.device+",";
            json=json+"method:"+t.method+",";
            json=json+"rgb:"+t.rgb;
            json=json+"}";
            if(array.equals("[")){
              array=array+json;
            }else{
              array=array+","+json;
            }
          }
      }
    }
    array=array+"]";
    queryData=array;
    r=1;
  }
  return r;
}

// print list of devices
String getDevices(){
  String devices="";
  int length = sizeof(deviceArr) / sizeof(deviceArr[0]);
  //length=length-1;
  String d=deviceArr[0].name;
  for(int i=0; i<length; i++){
    String d=deviceArr[i].name;
    if(devices==""){
      devices=d;
    }else{
      devices=devices+"|"+d;
    }
  }
  return devices;
}
// init pins 
void pinSetup(){
  int length = sizeof(deviceArr) / sizeof(deviceArr[0]);
  // loop through devices
  for(int i=0; i<length; i++){
    deviceS device=deviceArr[i];
    int componentArrLength=sizeof(device.components)/sizeof(device.components[0]);
    // loop through components of that device
    for(int a=0; a<componentArrLength; a++){
      component comp=device.components[a];
      if(comp.part!=""&&comp.pinmodeOut){
        pinMode(comp.pin,OUTPUT);
        // init rgb pins
        if(comp.rgb){
          // init pins 
          //red
          pinMode(comp.rgbPins[0],   OUTPUT);
          // green
          pinMode(comp.rgbPins[1],   OUTPUT);
          // blue
          pinMode(comp.rgbPins[2],   OUTPUT);
        }
      }else if(comp.part!=""&&comp.pinmodeIn){
        pinMode(comp.pin,INPUT);
      }else{
        break;
      }
      
    }
  }
}
void setRestDeviceVariables(){
  device=deviceSet.name;
  warning=deviceSet.warning;
  status=deviceSet.status;
  state=deviceSet.state;
}
void startSettings(){
  deviceSet=deviceArr[0];
  setRestDeviceVariables();
  pinSetup();
}
void setup(void)
{
  // Start Serial
  Serial.begin(115200);
  startSettings();
  // Init variables and expose them to REST API
  warning="",backgroundRunning=false,devices=getDevices(), uploadMode=false;
  queryData="";


  // global json
  rest.variable("Devices",&devices);
  // device framework
  rest.variable("Background",&backgroundRunning);
  rest.variable("QueryData",&queryData);
   // device readings
  rest.variable("SetDevice",&device);
  rest.variable("Warning",&warning);
  rest.variable("Status",&status);
  rest.variable("State",&state);
  // Function to be exposed
  // config route
  rest.function("upload",uploadModeConfig);
  rest.function("query",getDeviceData);
  rest.function("changeDevice",changeDevice);
  rest.function("command",command);
  // deviceRoutes
  rest.function("randomPermet",randPermet);
  rest.function("setPermet",setPermet);
  //randPermet



  // test route
  //rest.function("backgroundrun",backgroundrun);
  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("In1|3");
  rest.set_name("aREST Local");
   // TEMP
  
  //
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    pinMode(2,OUTPUT); 
    digitalWrite(2,0); 
    delay(500);
    Serial.print(".");
  }
  //
  // wireless upload
  ArduinoOTA.setPassword((const char *)"123");
   ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    pinMode(2,OUTPUT); 
    for(int a=0; a<3; a++){
      ledBlink(2,900);
    }
    pinMode(2,INPUT); 
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  //
  Serial.println("");
  Serial.println("WiFi connected");
  digitalWrite(2,1);
  pinMode(2,INPUT); 
  // Start the server
  server.begin();
  ArduinoOTA.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  // Reset server when upload mode true
  if(uploadMode==true){
    pinMode(2,OUTPUT); 
    ledBlink(2,900);
    ArduinoOTA.handle();
  }else{
  // Handle REST calls
  WiFiClient client = server.available();
  // Reset when cannot connect to wifi
  // Turn on D2 on board LED on when down
  if(WiFi.status()!= WL_CONNECTED){
    pinMode(2,OUTPUT); 
    digitalWrite(2,0);
  }else{
    digitalWrite(2,1);
    pinMode(2,INPUT); 
  }
  setRestDeviceVariables();
  background();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);
  }
}
// background processes 
void background(){
  // automatic
  if(queue.size()>0){
    backgroundRunning=true; 
  }else{
    backgroundRunning=false;
    task={};
  }
  
  if(backgroundRunning==true){
    
    for(int i=0; i<queue.size(); i++){
      task=queue[i];
      if(task.method=="ledBlinkBackGround"){
        ledBlinkBackGround(task.pin,task.interval);
        queue[i]=task;
      }
      if(task.method=="ledBlinkRgb"){
        ledBlinkBackGroundRgb(task.rgbRed,task.interval,task.pin);
        queue[i]=task;
      }
      if(task.method=="rgbCycle"){
        rgbCycle(task.interval);
        queue[i]=task;
      }
      if(task.method=="rgbFade"){
        rgbFade(task.interval);
        queue[i]=task;
      }
    }
    
  }
}








