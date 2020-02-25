#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include "arduino_secrets.h"

#define switch 8

//Encoder knobLeft(5, 6);
//Encoder knob1(2,3);
//Encoder knob2(4,5);
//Encoder knob3(6,7);
//Encoder encoder[3] = {knob1, knob2, knob3};
  
WiFiClient wifi;
String onApi = "  {\"on\": true}";
String offApi = "  {\"on\": false}";
boolean currentState = false;
boolean previousState = false;

String hubAddress[] = {"172.22.151.185",
                       "172.22.151.181",
                       "172.22.151.184"
                      };
String hubLogin[] = {"ISr8hVdpuJmmDs-COJb-d29KQD3tlgcdocc36eiD",
                     "ORMDpM9XmpurRMBt5eT0UrwWNM3VgChCZ08v8zcy",
                     "Jw-Si-Yh0VTqRw9l84uj7Koc0aSbOx3Zkhh65EeM"
                    };
int lightNumbers[] = {3, 1, 1};
HttpClient httpClient = HttpClient(wifi, hubAddress[0]);
int currentHub = 2;

long lastRequest[] = {10000,10000,10000,10000};
int requestDelay[] = {500,1500,1500,1500};

// switching handling
int switchState;
int pSwitchState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lightState;
int pLightState;

// rotary encoder handling
int output[][2] = {{2,3},
                  {4,5},
                  {6,7}};
int counter[] = {0,0,0}; 
int pCounter[] = {0,0,0};
int rotaryState[3];
int pRotaryState[3];
int sentCounter[] = {0,0,0};
bool sent[] = {false,false,false};

long positionLeft  = -999;
long positionRight = -999;


void setup() {
  Serial.begin(9600);
  // while (!Serial);

  pinMode(switch, INPUT);
  pinMode(output[0][0], INPUT);
  pinMode(output[0][1], INPUT);
  pinMode(output[1][0], INPUT);
  pinMode(output[1][1], INPUT);
  pinMode(output[2][0], INPUT);
  pinMode(output[2][1], INPUT);

  switchState = digitalRead(switch);
  if(switchState == HIGH){
    lightState = HIGH;
    pSwitchState = LOW;
    pLightState = LOW;
  }else{
    lightState = LOW;
    pSwitchState = HIGH;
    pLightState = HIGH;
  }

  pRotaryState[0] = digitalRead(output[0][0]);
  pRotaryState[1] = digitalRead(output[1][0]);
  pRotaryState[2] = digitalRead(output[2][0]);

  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(SECRET_SSID);
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(2000);
  }

  Serial.print("You're connected to the network. Your IP = ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
}

void loop() {
  while (httpClient.connected()) {
    // read response if available
    if (httpClient.available() > 0) {
      String response = httpClient.readString();
      Serial.println(response);
      Serial.println();
    }
  }

//  Modified version of SensorExamples/Encoders/Encoder_Button_Steps/Encoder_Button_Steps.ino
//  By Tom Igoe https://github.com/tigoe/SensorExamples/blob/master/Encoders/Encoder_Button_Steps/Encoder_Button_Steps.ino
  int switchState = digitalRead(switch);
  // if the button has changed:
  if (switchState != pSwitchState) {
    // debounce the button:
    delay(10);
    // if button is pressed:
    if (switchState == LOW) {
        lightState = LOW;
    } else {
      lightState = HIGH;
    }
    // save current state for next time through the loop:
    pSwitchState = switchState;
  }

  //
  if(lightState != pLightState){
    // wait till switch signal is stable
    if (millis() - lastRequest[0] > requestDelay[0]) {
      httpClient = HttpClient(wifi, hubAddress[currentHub]);
      if(lightState == HIGH){
        Serial.println(onApi);
        sendRequest(currentHub, lightNumbers[currentHub], onApi);
      }else if(lightState == LOW){
        Serial.println(offApi);
        sendRequest(currentHub, lightNumbers[currentHub], offApi);
      }
      // update last request timestamp:
      lastRequest[0] = millis();
    }
    // update previous light state
    pLightState = lightState;
  }


//  long newLeft, newRight;
//  newLeft = encoder[0].read();
//  newRight = encoder[1].read();
//  if (newLeft != positionLeft || newRight != positionRight) {
//    Serial.print("Left = ");
//    Serial.print(newLeft);
//    Serial.print(", Right = ");
//    Serial.print(newRight);
//    Serial.println();
//    positionLeft = newLeft;
//    positionRight = newRight;
//  }
//  // if a character is sent from the serial monitor,
//  // reset both back to zero.
//  if (Serial.available()) {
//    Serial.read();
//    Serial.println("Reset both knobs to zero");
//    encoder[0].write(0);
//    encoder[1].write(0);
//  }
//   read all 3 rotary encoders
  readRotary(0, "bri", 254);
  readRotary(1, "hue", 65535);
  readRotary(2, "sat", 254);
//  pSwitchState = reading;
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


/*     
 *  Modified and origianlly based on Arduino Rotary Encoder Tutorial
 *  by Dejan Nedelkovski, www.HowToMechatronics.com 
 */
void readRotary(int index, String prop, int cap){
  rotaryState[index] = digitalRead(output[index][0]);
  // detect rotary encoder state change
  if (rotaryState[index] != pRotaryState[index]){ 
    // If the another pin state is different to one pin, encoder is rotated clockwise
    if (digitalRead(output[index][1]) != rotaryState[index]) { 
      if(prop != "hue"){
        counter[index] ++;
      }else if(prop == "hue"){
        // special handing for huge hue value
        counter[index] = counter[index] + 100;
      }
    } else {
      if(prop != "hue"){
        counter[index] --;
      }else if(prop == "hue"){
        counter[index] = counter[index] - 100;
      }
    }

    // restrict floor
    if(counter[index] < 2 || counter[index] <= 0){
      counter[index] = 1;
    }
    // restrict ceiling
    if(counter[index] > cap){
      counter[index] = cap;
    }

    // update sent status
    sent[index] = false;
  }

  // if not rotating
  if(rotaryState[index] == pRotaryState[index]){
    // wait till the rotary encoder state to be stable
    if (millis() - lastRequest[index + 1] > requestDelay[index+1]) {
      // if value is new and not being sent
      if(sentCounter[index] != counter[index] && sent[index] == false){
        Serial.println("fire");
        httpClient = HttpClient(wifi, hubAddress[currentHub]);
        sendRequest(currentHub, lightNumbers[currentHub], propHelper(prop, counter[index]));
        // update sent status
        sent[index] = true;
        sentCounter[index] = counter[index];
      }
      // update last request timestamp:
      lastRequest[index+1] = millis();
    }
  }
 pRotaryState[index] = rotaryState[index];
}

// helper function for composing api body
String propHelper(String prop, int num){
    String state = "  {\"prop\": num}";
    state.replace("prop", prop);
    state.replace("num", String(num));
    return state;
}
