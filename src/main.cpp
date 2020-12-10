#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

///////////////////////// CUSTOM INCLUDES //////////////////////////////////
#include "MQTTLightControl.h"
#include "./Constants/Credentials.h"
#include <RCSwitch.h>

///////////////////////// CUSTOM INCLUDES //////////////////////////////////

const char *ssid = WIFI_SSID;
const char *password = WIFI_PSSWD;

///////////////////////// CUSTOM CODE ///////////////////////////////////////

char msg[50];
MQTTLightControl *mqttLightControl;
volatile bool msState = 0;

// PINS DECLARATION
const int RELAY_PIN = D7;//D1;
const int IR_REMOTE_PIN = D6;//D2; D8 - HANGS Wemos
//const int MS_PIN = D7;
const int MS_PIN = D0;
const int PR_PIN = A0; // Photoresistor
////////////

int lastIRActionTime = 0;


RCSwitch mySwitch = RCSwitch();

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String messageTemp = String((char*)payload);

  // for (int i = 0; i < length; i++)
  // {
  //   Serial.print((char)payload[i]);
  //   messageTemp += (char)payload[i];
  // }
  Serial.println(messageTemp);
  Serial.println();

  if (String(topic) == AUTHORIZATION_REQUESTS_STATUS_TPC)
  {
    mqttLightControl->authorizationResponse(messageTemp);
  }

  if (String(topic) == DEVICE_SETTINGS_APPLY_TPC)
  {
    mqttLightControl->applyNewSettings(messageTemp);
  }
  Serial.println();
}

///////////////////////// CUSTOM CODE ///////////////////////////////////////

void applyNewState() {
  if (mqttLightControl->getSettings().motionSensorSettings.relayMode)
    digitalWrite(RELAY_PIN, mqttLightControl->getState());
  else
    digitalWrite(RELAY_PIN, !mqttLightControl->getState());
}

// Interrupt callback
ICACHE_RAM_ATTR void msStateChange() {
  msState = digitalRead(MS_PIN);
  mqttLightControl->updateState(msState);
  applyNewState();
}

ICACHE_RAM_ATTR void msStateLow() {
  // msState = digitalRead(MS_PIN);
  mqttLightControl->updateState(0);
  applyNewState();
}




void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd uploading");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      Serial.println("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      Serial.println("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      Serial.println("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      Serial.println("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ///////////////////////// CUSTOM CODE ///////////////////////////////////////

  pinMode(MS_PIN, INPUT); // Initialize the BUILTIN_LED pin as an output
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PR_PIN, INPUT);

  //  Serial.begin(9600);

  mqttLightControl = new MQTTLightControl(
  //  (char *)MOTION_SENSOR_ID, 
    (char *)MQTT_SERVER, 1883,  
    ESP.getChipId());
  mqttLightControl->setCallback(callback);

  //msStateChange();
  attachInterrupt(digitalPinToInterrupt(MS_PIN), msStateChange, RISING);
  // attachInterrupt(digitalPinToInterrupt(MS_PIN), msStateLow, ONLOW);
  
  applyNewState();

  mqttLightControl->connect();
  mqttLightControl->authorizationRequest();

  mySwitch.enableReceive(digitalPinToInterrupt(IR_REMOTE_PIN));

  ///////////////////////// CUSTOM CODE ///////////////////////////////////////
}

void checkIRRemote() {
  if (mySwitch.available())
    {
      // if((millis() - lastIRActionTime) > 10) {
        lastIRActionTime = millis();

        int value = mySwitch.getReceivedValue();
        if (value == 0)
        {
          Serial.print("Unknown encoding");
        }
        else
        {

          Serial.print("Received ");
          Serial.println(mySwitch.getReceivedValue());
          Serial.print(" / ");
          Serial.print(mySwitch.getReceivedBitlength());
          Serial.print("bit ");
          Serial.print("Protocol: ");
          Serial.println(mySwitch.getReceivedProtocol());
        }

        if (mySwitch.getReceivedValue() == 16736113)
        {
          mqttLightControl->togglePreset();
          Serial.write("LOCK Button pressed");
        }
        if (mySwitch.getReceivedValue() == 11169970)
        {
          //mqttLightControl->applyPreset(1);
          mqttLightControl->increaseTimer();
          Serial.write("UNLOCK Button pressed");
        }
        if (mySwitch.getReceivedValue() == 16736114)
        {
          //mqttLightControl->applyPreset(2);
          mqttLightControl->decreaseTimer();
          Serial.write("HORN Button pressed");
        }
        if (mySwitch.getReceivedValue() == 16736120)
        {
          DeviceSettings newSettings = mqttLightControl->getSettings();
          
          switch(newSettings.operationMode) {
            case operationModes::MSDRIVEN: newSettings.operationMode = operationModes::ON; break;
            case operationModes::ON: newSettings.operationMode = operationModes::OFF; break;
            default: newSettings.operationMode = operationModes::MSDRIVEN; break;
          }
          
          mqttLightControl->setDeviceSettings(newSettings);
          Serial.write("OPEN TANK Button pressed");
        }
      // }
    

      mySwitch.resetAvailable();
    }
}

int readIllumination() {
  return analogRead(PR_PIN);
}

void loop()
{
  if(!WiFi.isConnected()) {
    Serial.println("WI-FI RECONNECTION");
    WiFi.begin(ssid, password);
  }

  ArduinoOTA.handle();

  ///////////////////////// CUSTOM CODE ///////////////////////////////////////

  mqttLightControl->keepAlive(MQTT_USER, MQTT_PSSWD);
  delay(10);         // Otherwise MQTT messages are not sent
  mqttLightControl->updateState(digitalRead(MS_PIN));

  applyNewState();

  checkIRRemote();

  
  mqttLightControl->updateIllumination(readIllumination());

  // Serial.print("Illumination: ");
  // Serial.println(illumination);
  mqttLightControl->showStatus();
  ///////////////////////// CUSTOM CODE ///////////////////////////////////////
}

