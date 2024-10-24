#ifndef STANDARD_FEATURES_H
#define STANDARD_FEATURES_H

#include "secrets.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "Logging.h"
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

WiFiClient espClient;
unsigned long wifiReconnectPreviousMillis = 0;
unsigned long wifiReconnectInterval = 30000;
uint8_t wifiReconnectCount = 0;

PubSubClient mqttClient(espClient);
uint32_t nextMqttConnectAttempt = 0;
const uint32_t mqttReconnectInterval = 10000;
uint32_t nextMetricsUpdate = 0;
bool mqttReconnected = false;

const uint8_t ONBOARD_LED_PIN = 13;
uint32_t nextOnboardLedUpdate = 0;
bool onboardLedState = true;

#ifdef DIAGNOSTIC_PIXEL_PIN
Adafruit_NeoPixel diagnosticPixel(1, DIAGNOSTIC_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint8_t diagnosticPixelMaxBrightness = 64;
uint8_t diagnosticPixelBrightness = diagnosticPixelMaxBrightness;
bool diagnosticPixelBrightnessDirection = 0;
uint32_t diagnosticPixelColor1 = 0xFF0000;
uint32_t diagnosticPixelColor2 = 0x000000;
uint32_t currentDiagnosticPixelColor = diagnosticPixelColor1;
uint32_t nextDiagnosticPixelUpdate = 0;
#endif

const uint32_t NEOPIXEL_BLACK = 0;
const uint32_t NEOPIXEL_RED =       Adafruit_NeoPixel::Color(255, 0,   0);
const uint32_t NEOPIXEL_ORANGE =    Adafruit_NeoPixel::Color(255, 165, 0);
const uint32_t NEOPIXEL_YELLOW =    Adafruit_NeoPixel::Color(255, 255, 0);
const uint32_t NEOPIXEL_MAGENTA =   Adafruit_NeoPixel::Color(255, 0,   255);
const uint32_t NEOPIXEL_GREEN =     Adafruit_NeoPixel::Color(0,   255, 0);
const uint32_t NEOPIXEL_CYAN =      Adafruit_NeoPixel::Color(0,   255, 255);
const uint32_t NEOPIXEL_BLUE =      Adafruit_NeoPixel::Color(0,   0,   255);
const uint32_t NEOPIXEL_WHITE =     Adafruit_NeoPixel::Color(255, 255, 255);

void connectToNetwork()
{
    WiFi.mode(WIFI_STA);
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
  
    ArduinoOTA.onStart([]()
    {
        Log.println("OTA Start");
        #ifdef DIAGNOSTIC_PIXEL_PIN
        diagnosticPixel.setPixelColor(0, NEOPIXEL_WHITE);
        diagnosticPixel.setBrightness(diagnosticPixelMaxBrightness);
        diagnosticPixel.show();
        #endif
    });

    ArduinoOTA.onEnd([]()
    {
        Log.println("OTA End");
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
    });
    
    ArduinoOTA.begin();
}

#ifdef DIAGNOSTIC_LED_PIN
void setupDiagnosticLed()
{
    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);
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
    diagnosticPixel.setPixelColor(0, NEOPIXEL_RED);
    diagnosticPixel.setBrightness(diagnosticPixelMaxBrightness);
    diagnosticPixel.show();
}

void manageDiagnosticPixel()
{
    if (millis() < nextDiagnosticPixelUpdate)
        return;

    if (mqttClient.connected())
        diagnosticPixelColor1 = NEOPIXEL_GREEN;
    else if (WiFi.status() == WL_CONNECTED)
        diagnosticPixelColor1 = NEOPIXEL_BLUE;
    else    
        diagnosticPixelColor1 = NEOPIXEL_RED;
    
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

    nextDiagnosticPixelUpdate = millis() + 33; // 33 = 30 FPS
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


void StandardSetup()
{
    Log.setup();

    #ifdef DIAGNOSTIC_LED_PIN
    setupDiagnosticLed();
    #endif

    #ifdef DIAGNOSTIC_PIXEL_PIN
    setupDiagnosticPixel();
    #endif

    setupMQTT();

    connectToNetwork();

    setupOTA();

    mqttConnect();
}

void StandardLoop()
{
    ArduinoOTA.handle();

    manageWiFi();

    manageMQTT();

    #ifdef DIAGNOSTIC_LED_PIN
    manageDiagnosticLed();
    #endif

    #ifdef DIAGNOSTIC_PIXEL_PIN
    manageDiagnosticPixel();
    #endif
}

#endif
