#pragma once

#include <Arduino.h>
#include <MQTTClient.h>
#include "Constants/Credentials.h"
#include <DeviceSettings.h>
#include "AuthorizationBlock.h"
#include <DumskyOLED.h>

class MQTTLightControl
{

public:
    MQTTLightControl(/*char *devId,*/ char *mqttServer, int mqttPort, uint32_t chipId);
    DeviceSettings getSettings();
    void setCallback(MQTT_CALLBACK_SIGNATURE);
    void connect();
    void keepAlive(char *mqttUsr, char *mqttPasswd);
    
    // ------------------------ Authorization ----------------------------
    void authorizationRequest();
    void authorizationResponse(String message);

    // -------------------------- Settings -------------------------------
    void applyNewSettings(String message);  // when MQTT message comes
    void setDeviceSettings(DeviceSettings newSettings); // setter
    // void setDefaultPresets();
    void applyPreset(int presetNumber);
    void togglePreset();
    void increaseTimer();
    void decreaseTimer();
    
    // -------------------------- Operation ------------------------------
    void updateState(int newState);
    int updateIllumination(int illumination);

    int getState();
    void setState(int newState);
    void resetTimer();
    void showStatus();


private:

    MQTTClient *mqttClient;

    boolean MSPreviousState = 0;
    boolean MSState = 0;

    // void sendMSState(boolean currState);
    // void sendOperationMode(int mode);

    operationModes decodeOperationMode(int intVar);
    void sendMSState();
    // void sendOperationMode();

    float startTimeOfOperation;
    int previousOperationCheckTime = 0;

    void msDrivenOperation(int newState);
    void offOperation();
    void onOperation();
    void settingsRequest();

    AuthorizationBlock authorizationBlock;
    DeviceSettings deviceSettings;
    std::vector<DeviceSettings> devicePresets;
    int currentPreset = 0;

    DumskyOLED oled;
    long lastScreenUpdateTime=0;
    long lastIllumTimeSend = 0;
    long lastMQTTKeepaliveLoop = 0;

    
};