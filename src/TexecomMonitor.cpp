#include "TexecomMonitor.h"

// Stubs
void zoneCallback(uint8_t zone, uint8_t state);
void alarmCallback(Texecom::ALARM_STATE state, uint8_t flags);

Texecom texecom(alarmCallback, zoneCallback);

void zoneCallback(uint8_t zone, uint8_t state)
{
    char attributesTopic[34];
    snprintf(attributesTopic, sizeof(attributesTopic), "home/security/zone/%03d", zone);
    char attributesMsg[46];

    snprintf(attributesMsg,
            sizeof(attributesMsg),
            "{\"active\":%d,\"tamper\":%d,\"fault\":%d,\"alarmed\":%d}",
            (state & Texecom::ZONE_ACTIVE) != 0,
            (state & Texecom::ZONE_TAMPER) != 0,
            (state & Texecom::ZONE_FAULT) != 0,
            (state & Texecom::ZONE_ALARMED) != 0);

    if (standardFeatures.isMQTTConnected())
    {
        standardFeatures.mqttPublish(attributesTopic, attributesMsg, true);
    }
}

void alarmCallback(Texecom::ALARM_STATE state, uint8_t flags)
{
    Log.printf("Alarm: %s\n", alarmStateStrings[state]);
    char message[58];
    snprintf(message,
                sizeof(message),
                "{\"state\":\"%s\",\"ready\":%d,\"fault\":%d,\"arm_failed\":%d}",
                alarmStateStrings[state],
                (flags & Texecom::ALARM_READY) != 0,
                (flags & Texecom::ALARM_FAULT) != 0,
                (flags & Texecom::ALARM_ARM_FAILED) != 0);
    standardFeatures.mqttPublish("home/security/alarm", message, true);
}
/*
void setupLocalMQTT()
{
}
*/
void setup()
{
    standardFeatures.enableLogging(deviceName, syslogServer, syslogPort);
    standardFeatures.enableWiFi(wifiSSID, wifiPassword, deviceName);
    standardFeatures.enableOTA(deviceName, otaPassword);
    standardFeatures.enableSafeMode(appVersion);
    standardFeatures.enableMQTT(mqttServer, mqttUsername, mqttPassword, deviceName);

    //setupLocalMQTT();

    texecom.setup();

    Log.println("Setup complete");
}

void loop()
{
    standardFeatures.loop();
    texecom.loop();
}