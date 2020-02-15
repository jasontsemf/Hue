#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include "arduino_secrets.h"

#define switch 4
#define outputA 2
#define outputB 3

WiFiClient wifi;
String onApi = "  {\"on\": true}";
String offApi = "  {\"on\": false}";
boolean currentState = false;
boolean previousState = false;
String hubAddress[] = {"172.22.151.185",
                       "172.22.151.181"
                      };
String hubLogin[] = {"ISr8hVdpuJmmDs-COJb-d29KQD3tlgcdocc36eiD",
                     "ORMDpM9XmpurRMBt5eT0UrwWNM3VgChCZ08v8zcy"
                    };
int lightNumbers[] = {3, 1};
HttpClient httpClient = HttpClient(wifi, hubAddress[0]);
int currentHub = 0;

long lastRequest[] = {10000,10000};
int requestDelay[] = {500,1500};

int switchState;
int pSwitchState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int ledState = LOW;         // the current state of the output pin
int lastLedState = HIGH;
int start = 0;

int counter = 0; 
int pCounter = 0;
int aState;
int aLastState;
int sentCounter = 0;
bool sent = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(switch, INPUT);
  pinMode (outputA,INPUT);
  pinMode (outputB,INPUT);
  
  switchState = digitalRead(switch);
  aLastState = digitalRead(outputA);
  if(switchState == HIGH){
    pSwitchState = LOW;
  }else{
    pSwitchState = HIGH;
  }
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(SECRET_SSID);
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(2000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network. Your IP = ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
}

void loop() {
  // clean up after the last set of requests by waiting for any remaining response:
  while (httpClient.connected()) {
    // print out whatever you get:
    if (httpClient.available() > 0) {
      String response = httpClient.readString();
      Serial.println(response);
      Serial.println();
    }
  }

  // read the state of the switch into a local variable:
  int reading = digitalRead(switch);

  // If the switch changed, due to noise or pressing:
  if (reading != pSwitchState && start > 0) {
    // reset the debouncing timer
    lastDebounceTime = millis();
    start++;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    // if the button state has changed:
    if (reading != switchState) {
      switchState = reading;

      // only toggle the LED if the new button state is HIGH
      if (switchState == HIGH) {
        ledState = HIGH;
      }else{
        ledState = LOW;
      }
    }
  }
  
  if(ledState != lastLedState){
    // set the LED:
    if (millis() - lastRequest[0] > requestDelay[0]) {
      httpClient = HttpClient(wifi, hubAddress[currentHub]);
      // make request:
  //    Serial.println(ledState);
      if(ledState == 1){
        Serial.println(onApi);
        sendRequest(currentHub, lightNumbers[currentHub], onApi);
      }else if(ledState == 0){
        Serial.println(offApi);
        sendRequest(currentHub, lightNumbers[currentHub], offApi);
      }
        
      // update the last request timestamp:
      lastRequest[0] = millis();
    }
    lastLedState = ledState;
  }
  readRotary();
  pSwitchState = reading;
  
}

void sendRequest(int hubNumber, int light, String myState) {
  // make a String for the HTTP request path:
  String request = "/api/" + String(hubLogin[hubNumber]);
  request += "/lights/";
  request += light;
  request += "/state/";

  String contentType = "application/json";

  // make a string for the JSON command:
  String body  = myState;

  // see what you assembled to send:
  Serial.print("PUT request to server: ");
  Serial.println(request);
  Serial.print("JSON command to server: ");
  Serial.println(body);

  // make the PUT request to the hub:
  httpClient.put(request, contentType, body);
}

void readRotary(){
  aState = digitalRead(outputA); // Reads the "current" state of the outputA
  // If the previous and the current state of the outputA are different, that means a Pulse has occured
  if (aState != aLastState){ 
    // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
    if (digitalRead(outputB) != aState) { 
      counter ++;
    } else {
      counter --;
    }
    if(counter < 2 || counter <= 0){
      counter = 1;
    }
    if(counter > 254){
      counter = 254;
    }
    Serial.print("Position: ");
    Serial.println(counter);
    sent = false;
  }
  if(aState == aLastState){
    if (millis() - lastRequest[1] > requestDelay[1]) {
      if(sentCounter != counter && sent == false){
        Serial.println("fire");
        httpClient = HttpClient(wifi, hubAddress[currentHub]);
        sendRequest(currentHub, lightNumbers[currentHub], lightBrightness(counter));
        sent = true;
        sentCounter = counter;
      }
      // update the last request timestamp:
      lastRequest[1] = millis();
    }
  }
 aLastState = aState;
}

String lightBrightness(int brightness){
    String state = "  {\"bri\": num}";
    state.replace("num", String(brightness));
    return state;
}
