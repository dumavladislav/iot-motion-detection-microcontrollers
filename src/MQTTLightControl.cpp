#include "MQTTLightControl.h"
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WifiConnect.h>
#include <MessageBuilder.h>
#include <MessageParser.h>
#include <DeviceSettings.h>

MQTTLightControl::MQTTLightControl(/*char *devId, */char *mqttServer, int mqttPort, uint32_t chipId)
{
  this->authorizationBlock.deviceId = deviceSettings.deviceId;
  this->authorizationBlock.chipId = chipId;
  WifiConnect* wifiConnect = new WifiConnect(WIFI_SSID, (char*) WIFI_PSSWD);
  wifiConnect->connect();

  Serial.println("MAC:");
  Serial.println(wifiConnect->getMacAddress());
  this->authorizationBlock.macAddress = wifiConnect->getMacAddress();
  this->authorizationBlock.ipAddress = wifiConnect->getIpAddress().toString();

  deviceSettings.deviceId = DEVICE_ID;
  Serial.print("Device ID");
  Serial.println(deviceSettings.deviceId.c_str());
  mqttClient = new MQTTClient((char*)(deviceSettings.deviceId.c_str()));
  mqttClient->init(mqttServer, mqttPort, wifiConnect->getNetworkClient(), PERIOD_MQTT_KEEPALIVE);
  setState(LOW);
  // setDefaultPresets();
  resetTimer();
  oled.init();
}

DeviceSettings MQTTLightControl::getSettings() {
  return this->deviceSettings;
}

void MQTTLightControl::setCallback(MQTT_CALLBACK_SIGNATURE)
{
  mqttClient->setCallback(callback);
}

void MQTTLightControl::connect()
{
  mqttClient->connect(MQTT_USER, MQTT_PSSWD);
  if(mqttClient->isConnected()) Serial.println("CONNECTED TO MQTT");
  else  Serial.println("FAILED TO CONNECT TO MQTT!");
  mqttClient->subscribe(AUTHORIZATION_REQUESTS_STATUS_TPC);   
  mqttClient->subscribe(DEVICE_SETTINGS_APPLY_TPC);
}

void MQTTLightControl::keepAlive(char *mqttUsr, char *mqttPasswd)
{
  // if((millis() - lastMQTTKeepaliveLoop) > PERIOD_MQTT_KEEPALIVE) {
    mqttClient->keepAlive(mqttUsr, mqttPasswd);
    // lastMQTTKeepaliveLoop = millis();
  // }
}

void MQTTLightControl::authorizationRequest() {

/*    const size_t jsonSize = JSON_OBJECT_SIZE(8);

    DynamicJsonDocument jsonDoc(jsonSize);

    float startTimeOnOperationMode = millis();
    String msgId = (char *)((String)authorizationBlock.deviceId 
    + String("_") + String(random(0xffff), HEX) 
    + String("_") + (String)startTimeOnOperationMode
    ).c_str();

    jsonDoc["msgId"] = msgId;
    jsonDoc["chipId"] = this->authorizationBlock.chipId;
    jsonDoc["macAddress"] = this->authorizationBlock.macAddress;
    jsonDoc["ipAddress"] = this->authorizationBlock.ipAddress;


    String jsonSerialized;
    serializeJson(jsonDoc, jsonSerialized);

    serializeJsonPretty(jsonDoc, Serial);
    mqttClient->sendMessage(AUTHORIZATION_REQUESTS_TPC, (char*)jsonSerialized.c_str());
      // ... and resubscribe
    mqttClient->subscribe(AUTHORIZATION_REQUESTS_STATUS_TPC); */
    MessageBuilder messageBuilder(authorizationBlock);
    messageBuilder.addElement("requestType", "authorization");
    if(!mqttClient->sendMessage(AUTHORIZATION_REQUESTS_TPC, messageBuilder.generateJson()))
      Serial.println("MESSAGE NOT SENT");
    else
    {
      Serial.println("AUTH MESSAGE SENT");
    }
    
    
}   

void MQTTLightControl::updateState(int newState)
{
  switch (deviceSettings.operationMode)
  {
  case operationModes::MSDRIVEN:
    msDrivenOperation(newState);
    break;
  case operationModes::ON:
    onOperation();
    break;
  case operationModes::OFF:
    offOperation();
    break;
  default:
    break;
  }
}

operationModes MQTTLightControl::decodeOperationMode(int intVar)
{
  operationModes enumVar = static_cast<operationModes>(intVar);
}

int MQTTLightControl::getState()
{
  return MSState;
}

void MQTTLightControl::setState(int newState) {
  MSState = newState;
}

void MQTTLightControl::sendMSState()
{
  //char message[50];
  //snprintf(message, 50, "%d", MSState);
  MessageBuilder messageBuilder(authorizationBlock);
  messageBuilder.addElement("msState", String(getState()));
  mqttClient->sendMessage(MOTION_TPC, messageBuilder.generateJson());
}

// void MQTTLightControl::sendOperationMode()
// {
//   char message[50];
//   snprintf(message, 50, "%d", deviceSettings.operationMode);
//   mqttClient->sendMessage(DEVICE_SETTINGS_APPLY_TPC, message);
// }

void MQTTLightControl::msDrivenOperation(int newState)
{
  if ((millis() - previousOperationCheckTime) > 50) {

    if (getState() != newState)
    {
      // Serial.println((millis() - startTimeOfOperation)/1000.0);
      // Serial.println(deviceSettings.motionSensorSettings.msDriveModeSettings.keepOnDurationSec);
      // Serial.println();
      if(
        newState == HIGH 
        || 
        (newState == LOW 
          and ((millis() - startTimeOfOperation) >= (deviceSettings.motionSensorSettings.msDriveModeDuration * 1000))
        )
      ) {
        setState(newState);
        resetTimer();
        sendMSState();
      }
    }
    previousOperationCheckTime = millis();
  }
}

void MQTTLightControl::offOperation()
{
  if ((millis() - previousOperationCheckTime) > 50) {
    if (getState() == HIGH)
    {
      setState(LOW);
      sendMSState();
      previousOperationCheckTime = millis();
    }
  }
}

void MQTTLightControl::onOperation()
{
  // int startTime = millis();
  if ((millis() - previousOperationCheckTime) > 50) {
    if (getState() == LOW)
    {
      resetTimer();
      setState(HIGH);
      sendMSState();
    }
    else
    {
      float currTime = millis();
      if ((currTime - startTimeOfOperation) >= (deviceSettings.motionSensorSettings.onModeDuration * 1000))
        this->deviceSettings.operationMode = operationModes::MSDRIVEN;
    }
    // Serial.print("onOperation duration: ");
    // Serial.println(millis() - startTime);
    previousOperationCheckTime = millis();
  }
}

void MQTTLightControl::authorizationResponse(String message) {
  const size_t jsonSize = JSON_OBJECT_SIZE(6) + 2*JSON_OBJECT_SIZE(2) + 120;
  DynamicJsonDocument jsonDoc(jsonSize);

  deserializeJson(jsonDoc, message);

  if(String((const char*)jsonDoc["header"]["macAddress"]) == this->authorizationBlock.macAddress) {

    this->authorizationBlock.authorized = jsonDoc["data"]["authorized"];
    this->authorizationBlock.securityToken = (const char *)jsonDoc["data"]["sessionToken"];

    settingsRequest();
  }
}

void MQTTLightControl::settingsRequest() {

  MessageBuilder messageBuilder(authorizationBlock);
  messageBuilder.addElement("requestType", "settings");
  mqttClient->sendMessage(DEVICE_SETTINGS_REQUEST_TPC, messageBuilder.generateJson());
}

void MQTTLightControl::applyNewSettings(String message) {
  Serial.println("New settings received!");
  MessageParser messageParser(authorizationBlock);
  if(messageParser.parseIncomingMessage(message.c_str())) {
    JsonObject data = messageParser.getDataBlock();
    int i = 0;
    for(JsonObject preset : (JsonArray)data["deviceSettings"]["settingsPresets"]) {
      DeviceSettings newSettings;
      newSettings.name = (const char*) preset["name"];
      newSettings.defaultPreset = (boolean)preset["defaultPreset"];
      newSettings.operationMode = (operationModes)((int)preset["operationMode"]);
      newSettings.motionSensorSettings.relayMode = preset["motionSensorSettings"]["relayMode"];
      newSettings.motionSensorSettings.msDriveModeDuration = (int)preset["motionSensorSettings"]["msDriveModeDuration"];
      newSettings.motionSensorSettings.onModeDuration = (int)preset["motionSensorSettings"]["onModeDuration"];
      devicePresets.push_back(newSettings);
      if ( newSettings.defaultPreset )  this->setDeviceSettings(newSettings);
    }
  }
  Serial.println("New settings applied!");
}

void MQTTLightControl::setDeviceSettings(DeviceSettings newSettings) {
  // int startTime = millis();
  this->deviceSettings = newSettings;
  Serial.print("New mode: ");
  Serial.println(deviceSettings.operationMode);
  resetTimer();
  // Serial.print("setDeviceSettings duration: ");
  // Serial.println(millis() - startTime);

}

/*void MQTTLightControl::setDefaultPresets() {
  // for(int i=0; i<devicePresets.size(); i++)
  // this->devicePresets.
  devicePresets.assign(4, deviceSettings);

  devicePresets[0].operationMode = operationModes::MSDRIVEN;

  devicePresets[1].operationMode = operationModes::ON;

  devicePresets[2].operationMode = operationModes::OFF;
  
}*/

void MQTTLightControl::applyPreset(int presetNumber) {
  setDeviceSettings(devicePresets[presetNumber]);
}

void MQTTLightControl::increaseTimer() {
  switch(deviceSettings.operationMode) {
    case operationModes::MSDRIVEN:
      deviceSettings.motionSensorSettings.msDriveModeDuration += 5; 
      break;
    case operationModes::ON:
      deviceSettings.motionSensorSettings.onModeDuration +=10;
      break;
  }
  // Serial.println();
  // Serial.print("New Timer value: ");
  // Serial.println(deviceSettings.motionSensorSettings.msDriveModeDuration);
  // Serial.println(deviceSettings.motionSensorSettings.onModeDuration);
}

void MQTTLightControl::togglePreset() {
  if (devicePresets.size() > 0) {
    if (currentPreset >= (devicePresets.size() - 1)) currentPreset = 0;
    else currentPreset++;

    applyPreset(currentPreset);
  }
}

void MQTTLightControl::decreaseTimer() {
  switch(deviceSettings.operationMode) {
    case operationModes::MSDRIVEN:
      if(deviceSettings.motionSensorSettings.msDriveModeDuration > 5)
        deviceSettings.motionSensorSettings.msDriveModeDuration -= 5; 
      break;
    case operationModes::ON:
      if(deviceSettings.motionSensorSettings.onModeDuration > 10)
        deviceSettings.motionSensorSettings.onModeDuration -=10;
      break;
  }
  // Serial.println();
  // Serial.print("New Timer value: ");
  // Serial.println(deviceSettings.motionSensorSettings.msDriveModeDuration);
  // Serial.println(deviceSettings.motionSensorSettings.onModeDuration);
}

void MQTTLightControl::resetTimer() {
  startTimeOfOperation = millis();
  previousOperationCheckTime = millis();
}

void MQTTLightControl::showStatus() {
  if((millis() - lastScreenUpdateTime) > 500) {
    // int startTime = millis();
    // Serial.println();
    // Serial.print(millis());
    // Serial.println(" Updating the screen");
    oled.clear();
    DeviceSettings settings = getSettings();
    String statusString = "";
    int timer = 0;
    
    
    switch(settings.operationMode) {
      case operationModes::MSDRIVEN: {
        oled.addString("MS  " + String(settings.motionSensorSettings.msDriveModeDuration) + "  " + (MSState == 1 ? String(settings.motionSensorSettings.msDriveModeDuration - ((millis() - startTimeOfOperation)/1000),0) : ""));
        break;
      }
      case operationModes::ON: {
        oled.addString("ON  " + String(settings.motionSensorSettings.onModeDuration/60) + "  " + String((settings.motionSensorSettings.onModeDuration - ((millis() - startTimeOfOperation)/1000))/60,0));
        break;
      }
      case operationModes::OFF: {
        oled.addString("OFF");
        break;
      }
    }

    /*String msStateStr = (MSState ? "ON " : "OFF");
    
    switch(settings.operationMode) {
      case operationModes::MSDRIVEN: {
        oled.addString("MS | " + msStateStr);
        if (MSState == 1) {
          oled.addString("Timer: "+ String(settings.motionSensorSettings.msDriveModeDuration) + " | Left: " + String(settings.motionSensorSettings.msDriveModeDuration - ((millis() - startTimeOfOperation)/1000),0));
        }
        break;
      }
      case operationModes::ON: {
        oled.addString("ON | " + msStateStr);
        if (MSState == 1) {
          oled.addString("Timer: "+ String(settings.motionSensorSettings.onModeDuration) + " | Left: " + String(settings.motionSensorSettings.onModeDuration - ((millis() - startTimeOfOperation)/1000),0));
        }
        break;
      }
      case operationModes::OFF: {
        oled.addString("OFF | " + msStateStr);
        break;
      }
    }*/
    
    oled.show();
    lastScreenUpdateTime = millis();
    // Serial.print("showStatus duration: ");
    // Serial.println(millis() - startTime);
  }
}

int MQTTLightControl::updateIllumination(int illumination) {
  if ((millis() - lastIllumTimeSend) > (PERIOD_ILLUMINATION_READ * 1000))
  {
    MessageBuilder messageBuilder(authorizationBlock);
    messageBuilder.addElement("illumination", String(illumination));
    mqttClient->sendMessage(ILLUMINATION_TPC, messageBuilder.generateJson());
    lastIllumTimeSend = millis();
    
  }
}