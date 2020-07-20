#include <Arduino.h>
#include "MQTTLightControl.h"


#include <iostream>
#include <ctime>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>  


#include "Credentials.h"
#include "Constants.h"

// Update these with values suitable for your network.
char msg[50];

MQTTLightControl mqttLightControl((char*) MOTION_SENSOR_ID, (char*) MQTT_SERVER, 1883/*, callback*/);

int MSValue = 0;

// PINS DECLARATION
const int RELAY_PIN = D1;
const int MS_PIN = A0;

////////////

using namespace std;
vector<string> split(string str, char delimiter) {
  vector<string> internal;
  stringstream ss(str); // Turn the string into a stream.
  string tok;
 
  while(getline(ss, tok, delimiter)) {
    internal.push_back(tok);
  }
 
  return internal;
}

/*
void sendMessage(char* topicName, char message[]) {
  // current date/time based on current system
  time_t now = time(0);
   
  // convert now to string form
  char* dt = ctime(&now);

  snprintf (msg, 50, "%s%s: %s", dt, MOTION_SENSOR_ID, message);
  client.publish(topicName, msg);
}

void sendMSState(boolean currState) {
  char message[50];
  snprintf (message, 50, "%d", currState);
  sendMessage(MOTION_SENSOR_STATE_TPC, message);
}

void sendOperationMode(int mode) {
  char message[50];
  snprintf (message, 50, "%d", mode);
  sendMessage(DEVICE_OPERATION_MODE_TPC, message);
}*/

/*
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PSSWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}*/

/*

void setOperationMode(int mode) {
  Serial.print("DEVICE ");
  Serial.print(MOTION_SENSOR_ID);
  Serial.print(": Changing operation mode to ");
  Serial.println(mode);
  operationMode = getOperationMode(mode);
  sendOperationMode(operationMode);
}*/


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String messageTemp;
   
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();
 
  if(topic==DEVICE_OPERATION_CONTROL_TPC){
     
      vector<string> sep = split(messageTemp.c_str(), ':');
      if (sep[0].c_str() == MOTION_SENSOR_ID) {
        mqttLightControl.setOperationMode(atoi(sep[1].c_str()));
      }
  }
  Serial.println();
}



/*
void reconnect(String clientId) {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PSSWD)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(GREETING_TPC, (clientId + " connected").c_str());
      // ... and resubscribe
      client.subscribe(DEVICE_OPERATION_CONTROL_TPC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
*/

void setup() {
  pinMode(MS_PIN, INPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(RELAY_PIN, OUTPUT);
  
  //digitalWrite(RELAY_PIN, HIGH);
  Serial.begin(9600);

  //mqttLightControl = MQTTLightControl((char*) MOTION_SENSOR_ID, (char*) MQTT_PORT, 1883, callback);

/*  setup_wifi();
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);  */
}


/*
void msDrivenOperation() {
    MSValue = analogRead(MS_PIN);

    Serial.print("MSValue = ");
    Serial.println(MSValue);

    if (MSValue >= 333) MSState = HIGH;
    else MSState = LOW;

    //boolean RelayState = digitalRead(RELAY_SCAN);
    //Serial.println("MSState = " + MSState);


    if (MSState != MSPreviousState) {
      MSPreviousState = MSState;
      Serial.println("State changed!");
      digitalWrite(RELAY_PIN, !MSState);
      sendMSState(MSState);
    }
}

void offOperation() {
  if(MSState == HIGH) {
    MSState = LOW;
    digitalWrite(RELAY_PIN, !MSState);
    sendMSState(MSState);
  }
}

void onOperation() {
  if(MSState == LOW) {
    startTimeOnOperationMode = millis();
    MSState = HIGH;
    digitalWrite(RELAY_PIN, !MSState);
    sendMSState(MSState);
  }
  else
  {
    float currTime = millis();
    if((currTime - startTimeOnOperationMode) >= MAX_ON_OPERATION_MODE_DURATION_MS) offOperation();
  }
  
}*/


void loop() {

  // if (!client.connected()) {
  //   reconnect(MOTION_SENSOR_ID + String(random(0xffff), HEX));
  // }
  // client.loop();

  mqttLightControl.setCallback(callback);

  mqttLightControl.keepAlive(MQTT_USER, MQTT_PSSWD);
  delay(10);
  mqttLightControl.updateState(analogRead(MS_PIN));
  Serial.print("Analog sygnal: ");
  Serial.println(analogRead(MS_PIN));
  if(RELAY_HIGH) digitalWrite(RELAY_PIN, mqttLightControl.getState());
  else digitalWrite(RELAY_PIN, !(mqttLightControl.getState()));
}

