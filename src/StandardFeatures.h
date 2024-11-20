#ifndef STANDARD_FEATURES_H
#define STANDARD_FEATURES_H

#include "secrets.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "Logging.h"
#include "Preferences.h"
#include <PubSubClient.h>

Preferences standardPreferences;

WiFiClient espClient;
unsigned long wifiReconnectPreviousMillis = 0;
unsigned long wifiReconnectInterval = 30000;
uint8_t wifiReconnectCount = 0;

PubSubClient mqttClient(espClient);
uint32_t nextMqttConnectAttempt = 0;
const uint32_t mqttReconnectInterval = 10000;
uint32_t nextMetricsUpdate = 0;
bool mqttReconnected = false;
bool OTA_RUNNING = false;
bool ENABLE_WIFI = true;

const char *prefBadBootCount = "bad_boot_count";
const char *prefAppVersion = "app_version";
const char *prefBootSuccess = "boot_success";

const uint16_t safeModeGoodBootAfterTime = 30000;
uint32_t safeModeSafeTime = 0;
const uint8_t  safeModeBadBootTrigger = 3;
bool safeModeBootIsGood = false;
uint8_t safeModeBadBootCount = 0;

#ifdef DIAGNOSTIC_LED_PIN
const uint8_t ONBOARD_LED_PIN = DIAGNOSTIC_LED_PIN;
uint32_t nextOnboardLedUpdate = 0;
bool onboardLedState = true;
#endif

#ifdef DIAGNOSTIC_PIXEL_PIN
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel diagnosticPixel(1, DIAGNOSTIC_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

const uint32_t NEOPIXEL_BLACK =     0;
const uint32_t NEOPIXEL_RED =       Adafruit_NeoPixel::Color(255, 0,   0);
const uint32_t NEOPIXEL_ORANGE =    Adafruit_NeoPixel::Color(255, 140, 0);
const uint32_t NEOPIXEL_YELLOW =    Adafruit_NeoPixel::Color(255, 255, 0);
const uint32_t NEOPIXEL_MAGENTA =   Adafruit_NeoPixel::Color(255, 0,   255);
const uint32_t NEOPIXEL_GREEN =     Adafruit_NeoPixel::Color(0,   255, 0);
const uint32_t NEOPIXEL_CYAN =      Adafruit_NeoPixel::Color(0,   255, 255);
const uint32_t NEOPIXEL_BLUE =      Adafruit_NeoPixel::Color(0,   0,   255);
const uint32_t NEOPIXEL_WHITE =     Adafruit_NeoPixel::Color(255, 255, 255);

uint8_t diagnosticPixelMaxBrightness = 64;
uint8_t diagnosticPixelBrightness = diagnosticPixelMaxBrightness;
bool diagnosticPixelBrightnessDirection = 0;
uint32_t diagnosticPixelColor1 = NEOPIXEL_BLUE;
uint32_t diagnosticPixelColor2 = NEOPIXEL_BLACK;
uint32_t currentDiagnosticPixelColor = diagnosticPixelColor1;
uint32_t nextDiagnosticPixelUpdate = 0;
uint32_t diagnosticPixelUpdateGap = 1500 / diagnosticPixelMaxBrightness;
#endif

void StandardSetup();
void StandardLoop();

void connectToNetwork()
{
    WiFi.mode(WIFI_STA);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(deviceName);
    WiFi.begin(wifiSSID, wifiPassword);

    if (WiFi.waitForConnectResult() == WL_CONNECTED && wifiReconnectCount == 0)
        Log.println("Connected to WiFi");
}

void manageWiFi()
{
    // if WiFi is down, try reconnecting
    if ((WiFi.status() != WL_CONNECTED) && (millis() - wifiReconnectPreviousMillis >= wifiReconnectInterval))
    {
        if (wifiReconnectCount >= 10)
        {
            ESP.restart();
        }
        
        wifiReconnectCount++;

        connectToNetwork();

        if (WiFi.status() == WL_CONNECTED)
        {
            wifiReconnectCount = 0;
            wifiReconnectPreviousMillis = 0;
            Log.println("Reconnected to WiFi");
        }
        else
        {
            wifiReconnectPreviousMillis = millis();
        }
    }
}

void setupMQTT()
{
    mqttClient.setBufferSize(4096);
    mqttClient.setServer(mqtt_server, 1883);
}

void mqttConnect()
{
    Log.println("Connecting to MQTT");
    // Attempt to connect
    if (mqttClient.connect(deviceName, mqtt_username, mqtt_password))
    {
        Log.println("Connected to MQTT");
        nextMqttConnectAttempt = 0;
        mqttReconnected = true;
    }
    else
    {
        Log.println("Failed to connect to MQTT");
        nextMqttConnectAttempt = millis() + mqttReconnectInterval;
    }
}

void setupOTA()
{
    ArduinoOTA.setHostname(deviceName);
    ArduinoOTA.setPassword(otaPassword);

    ArduinoOTA.onStart([]()
    {
        Log.println("OTA Start");
        OTA_RUNNING = true;
        #ifdef DIAGNOSTIC_PIXEL_PIN
        diagnosticPixel.setPixelColor(0, NEOPIXEL_WHITE);
        diagnosticPixel.setBrightness(diagnosticPixelMaxBrightness);
        diagnosticPixel.show();
        #endif
    });

    ArduinoOTA.onEnd([]()
    {
        Log.println("OTA End");
        OTA_RUNNING = false;
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
        {
            Log.println("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            Log.println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            Log.println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            Log.println("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
            Log.println("End Failed");
        }
        OTA_RUNNING = false;
    });
    
    ArduinoOTA.begin();
}

void setupSafeMode()
{
    bool lastBootState = true; // Lets assume success
    String lastAppVersion = standardPreferences.getString(prefAppVersion, "");

    if (!lastAppVersion.equals(appVersion))
    {
        // If there's a new OS lets not judge it too quickly. Reset the bad boot count and lastBootState
        standardPreferences.putString(prefAppVersion, appVersion);
        standardPreferences.putUChar(prefBadBootCount, 0);
    }
    else
    {
        lastBootState = standardPreferences.getBool(prefBootSuccess, true);
    }

    if (!lastBootState)
    {
        safeModeBadBootCount = standardPreferences.getUChar(prefBadBootCount, 0) + 1;
        Log.printf("bad_boot_count = %d\n", safeModeBadBootCount);
        if (safeModeBadBootCount >= 3)
        {
            // This should be safe mode
            Log.println("Entered safe mode");
            while(true)
            {
                #ifdef DIAGNOSTIC_PIXEL_PIN
                diagnosticPixelColor2 = NEOPIXEL_RED;
                #endif
                StandardLoop();
            }
        }
        else
        {
            standardPreferences.putUChar(prefBadBootCount, safeModeBadBootCount);
        }
    }

    // Write it's a bad boot and we'll say it's good after a defined period of time.
    standardPreferences.putBool(prefBootSuccess, false);

    safeModeSafeTime = millis() + safeModeGoodBootAfterTime;
}

void manageSafeMode()
{
    if (!safeModeBootIsGood && safeModeBadBootCount < safeModeBadBootTrigger && millis() > safeModeSafeTime)
    {
        safeModeBootIsGood = true;
        standardPreferences.putBool(prefBootSuccess, true);
        Log.println("Booted safely");

        if (safeModeBadBootCount > 0)
        {
            standardPreferences.putUChar(prefBadBootCount, 0);
        }
    }
}

#ifdef DIAGNOSTIC_LED_PIN
void setupDiagnosticLed()
{
    pinMode(ONBOARD_LED_PIN, OUTPUT);
    digitalWrite(ONBOARD_LED_PIN, HIGH);
}

void manageDiagnosticLed()
{
    if (millis() < nextOnboardLedUpdate)
        return;

    nextOnboardLedUpdate = millis() + 1000;
    onboardLedState = !onboardLedState;
    digitalWrite(ONBOARD_LED_PIN, onboardLedState ? HIGH : LOW);
}
#endif

#ifdef DIAGNOSTIC_PIXEL_PIN
void setupDiagnosticPixel()
{   
    diagnosticPixel.begin();
    diagnosticPixel.setPixelColor(0, NEOPIXEL_BLUE);
    diagnosticPixel.setBrightness(diagnosticPixelMaxBrightness);
    diagnosticPixel.show();
}

void setMaxDiagnosticPixelBrightness(uint8_t brightness)
{
    diagnosticPixelMaxBrightness = brightness;
    diagnosticPixelUpdateGap = 1500 / brightness;
}

void manageDiagnosticPixel()
{
    if (millis() < nextDiagnosticPixelUpdate)
        return;

    if (mqttClient.connected())
        diagnosticPixelColor1 = NEOPIXEL_GREEN;
    else if (WiFi.status() == WL_CONNECTED)
        diagnosticPixelColor1 = NEOPIXEL_ORANGE;
    else    
        diagnosticPixelColor1 = NEOPIXEL_BLUE;
    
    if (diagnosticPixelBrightness <= 0)
    {
        diagnosticPixelBrightnessDirection = 1;
        if (diagnosticPixelColor2 != NEOPIXEL_BLACK && currentDiagnosticPixelColor == diagnosticPixelColor1)
            currentDiagnosticPixelColor = diagnosticPixelColor2;
        else
            currentDiagnosticPixelColor = diagnosticPixelColor1;
    }
    else if (diagnosticPixelBrightness >= diagnosticPixelMaxBrightness)
    {
        diagnosticPixelBrightnessDirection = 0;
    }
    
    diagnosticPixelBrightness = diagnosticPixelBrightnessDirection ? diagnosticPixelBrightness+1 : diagnosticPixelBrightness-1;
    diagnosticPixel.setPixelColor(0, currentDiagnosticPixelColor);
    diagnosticPixel.setBrightness(diagnosticPixelBrightness);
    diagnosticPixel.show();

    nextDiagnosticPixelUpdate = millis() + diagnosticPixelUpdateGap;
}
#endif

void sendTelegrafMetrics()
{
    if (millis() > nextMetricsUpdate)
    {
        nextMetricsUpdate = millis() + 30000;
        uint32_t uptime = esp_timer_get_time() / 1000000;

        char buffer[150];
        snprintf(buffer, sizeof(buffer),
            "status,device=%s uptime=%d,resetReason=%d,firmware=\"%s\",memUsed=%ld,memTotal=%ld",
            deviceName,
            uptime,
            esp_reset_reason(),
            esp_get_idf_version(),
            (ESP.getHeapSize()-ESP.getFreeHeap()),
            ESP.getHeapSize());
        mqttClient.publish("telegraf/particle", buffer);
    }
}

void manageMQTT()
{
    if (mqttClient.connected())
    {
        mqttClient.loop();

        if (millis() > nextMetricsUpdate)
        {
            sendTelegrafMetrics();
            nextMetricsUpdate = millis() + 30000;
        }
    }
    else if (millis() > nextMqttConnectAttempt)
    {
        if (WiFi.status() == WL_CONNECTED)
            mqttConnect();
    }
}

void startWiFi()
{
    ENABLE_WIFI = true;

    Log.disableSyslog(false);

    connectToNetwork();

    setupOTA();

    mqttConnect();
}

void stopWiFi()
{
    if (!ENABLE_WIFI)
        return;

    ENABLE_WIFI = false;

    Log.disableSyslog(true);

    ArduinoOTA.end();

    mqttClient.disconnect();

    WiFi.disconnect();
}

void StandardSetup()
{
    standardPreferences.begin("standardfeature", false);

    Log.setup();

    #ifdef DIAGNOSTIC_LED_PIN
    setupDiagnosticLed();
    #endif

    #ifdef DIAGNOSTIC_PIXEL_PIN
    setupDiagnosticPixel();
    #endif

    setupMQTT();

    if (ENABLE_WIFI)
    {
        startWiFi();
    }

    // Should always be last
    setupSafeMode();
}

void StandardLoop()
{
    if (ENABLE_WIFI)
    {
        ArduinoOTA.handle();

        manageWiFi();

        manageMQTT();
    }

    #ifdef DIAGNOSTIC_LED_PIN
    manageDiagnosticLed();
    #endif

    #ifdef DIAGNOSTIC_PIXEL_PIN
    manageDiagnosticPixel();
    #endif

    manageSafeMode();
}

#endif
