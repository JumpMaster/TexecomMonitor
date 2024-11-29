#ifndef STANDARD_FEATURES_H
#define STANDARD_FEATURES_H

//#define DIAGNOSTIC_PIXEL
//#define DIAGNOSTIC_LED

#include <Arduino.h>
#include <functional>
#include <ArduinoOTA.h>
#include "Logging.h"
#include "Preferences.h"
#include <WiFi.h>
#include <PubSubClient.h>

#ifdef DIAGNOSTIC_PIXEL
#include <Adafruit_NeoPixel.h>
#endif

class StandardFeatures
{

public:
    StandardFeatures();
    ~StandardFeatures();

    void loop();

    void enableSafeMode(const char *appVersion);
    void enableLogging(const char *appName, const char *syslogServer, uint16_t sysLogPort = 514);
    void enableDiagnosticPixel(uint8_t pin);
    void enableDiagnosticLed(uint8_t pin);
    void enableWiFi(const char *wifiSSID, const char *wifiPassword, const char *deviceName);
    void disableWiFi();
    void enableMQTT(const uint8_t *mqttServer, const char *mqttUsername, const char *mqttPassword, const char *mqttDeviceName);
    void disableMQTT();
    void enableOTA(const char *hostname, const char *otaPassword);
    void disableOTA();
    bool isOTARunning() { return otaRunning; }
    bool isWiFiEnabled() { return _wifiEnabled; }
    bool isOTAEnabled() { return _otaEnabled; }
    bool isMQTTEnabled() { return _mqttEnabled; }
    bool isMQTTConnected() { return (_mqttEnabled && _mqttClient->connected()); }
    void setDiagnosticPixelColor(uint32_t color);
    void setMqttOnConnectCallback(void (*callback)());
    void setMqttCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);
    bool mqttPublish(const char* topic, const char* payload, boolean retained);
    bool mqttSubscribe(const char* topic);

    static const uint32_t NEOPIXEL_BLACK =     0;
    static const uint32_t NEOPIXEL_RED =       16711680;
    static const uint32_t NEOPIXEL_ORANGE =    16747520;
    static const uint32_t NEOPIXEL_YELLOW =    16776960;
    static const uint32_t NEOPIXEL_MAGENTA =   16711935;
    static const uint32_t NEOPIXEL_GREEN =     65280;
    static const uint32_t NEOPIXEL_CYAN =      65535;
    static const uint32_t NEOPIXEL_BLUE =      255;
    static const uint32_t NEOPIXEL_WHITE =     16777215;

private:

protected:
    void setupDiagnosticPixel();
    void manageDiagnosticPixel();
    void manageDiagnosticLed();
    void connectToNetwork();
    void manageWiFi();
    void connectToMQTT();
    void manageMQTT();
    void manageSafeMode();
    void sendTelegrafMetrics();
    void setDiagnosticLEDUpdateTime(uint16_t pause);
    void mqttCallback(char* topic, byte* payload, unsigned int length);

#ifdef DIAGNOSTIC_PIXEL
    uint8_t diagnosticPixelMaxBrightness = 64;
    uint8_t diagnosticPixelBrightness = diagnosticPixelMaxBrightness;
    bool diagnosticPixelBrightnessDirection = 0;
    uint32_t diagnosticPixelColor1 = NEOPIXEL_BLUE;
    uint32_t diagnosticPixelColor2 = NEOPIXEL_BLACK;
    uint32_t currentDiagnosticPixelColor = diagnosticPixelColor1;
    uint32_t nextDiagnosticPixelUpdate = 0;
    uint32_t diagnosticPixelUpdateGap = 40;
    uint8_t _diagnosticPixelPin = 0;
    Adafruit_NeoPixel *_diagnosticPixel;
#endif

#ifdef DIAGNOSTIC_LED
    uint32_t _nextDiagnosticLedUpdate = 0;
    uint16_t _diagnosticLedUpdateTime = 1000;
    uint8_t _diagnosticLedPin;
    bool _diagnosticLedState = false;
#endif

    bool _diagnosticLedEnabled = false;
    bool _diagnosticPixelEnabled = false;
    bool _wifiEnabled = false;
    bool _mqttEnabled = false;
    bool _safeModeEnabled = false;
    bool _metricsEnabled = false;
    bool _otaEnabled = false;

    const char *_wifiSSID = "";
    const char *_wifiPassword = "";
    const char *_deviceName = "";
    const char *_appVersion = "";

    WiFiClient espClient;
    unsigned long wifiReconnectPreviousMillis = 0;
    unsigned long wifiReconnectInterval = 30000;
    uint8_t wifiReconnectCount = 0;

    uint32_t nextMqttConnectAttempt = 0;
    const uint32_t mqttReconnectInterval = 10000;
    uint32_t nextMetricsUpdate = 0;

    PubSubClient *_mqttClient;
    const uint8_t *_mqttServer; // MQTT Server IP Address
    const char *_mqttUsername = "";
    const char *_mqttPassword = "";
    const char *_mqttDeviceName = "";
    std::function<void()> _onMQTTConnectCallback;

    Preferences *standardPreferences;
    const char *prefBadBootCount = "bad_boot_count";
    const char *prefAppVersion = "app_version";
    const char *prefBootSuccess = "boot_success";

    uint16_t safeModeGoodBootAfterTime = 30000;
    uint32_t safeModeSafeTime = 0;
    const uint8_t  safeModeBadBootTrigger = 3;
    bool safeModeBootIsGood = false;
    uint8_t safeModeBadBootCount = 0;

    bool otaRunning = false;
};


StandardFeatures::StandardFeatures()
{
    standardPreferences = new Preferences();
    standardPreferences->begin("standardfeature", false);
    setMqttOnConnectCallback(NULL);
}

StandardFeatures::~StandardFeatures()
{
    standardPreferences->end();
}

void StandardFeatures::enableLogging(const char *appName, const char *syslogServer, uint16_t sysLogPort)
{
    Log.enableLogging(appName, syslogServer, sysLogPort);
}

void StandardFeatures::enableSafeMode(const char *appVersion)
{
    _safeModeEnabled = true;
    _appVersion = appVersion;
    standardPreferences = new Preferences();
    standardPreferences->begin("standardfeature", false);

    bool lastBootState = true; // Lets assume success
    String lastAppVersion = standardPreferences->getString(prefAppVersion, "");

    if (!lastAppVersion.equals(_appVersion))
    {
        // If there's a new OS lets not judge it too quickly. Reset the bad boot count and lastBootState
        standardPreferences->putString(prefAppVersion, _appVersion);
        standardPreferences->putUChar(prefBadBootCount, 0);
    }
    else
    {
        lastBootState = standardPreferences->getBool(prefBootSuccess, true);
    }

    if (!lastBootState)
    {
        safeModeBadBootCount = standardPreferences->getUChar(prefBadBootCount, 0) + 1;
        Log.printf("bad_boot_count = %d\n", safeModeBadBootCount);
        if (safeModeBadBootCount >= 3)
        {
            // This should be safe mode
            Log.println("Entered safe mode");
            setDiagnosticPixelColor(NEOPIXEL_RED);
            setDiagnosticLEDUpdateTime(250);
            while(true)
            {
                loop();
            }
        }
        else
        {
            standardPreferences->putUChar(prefBadBootCount, safeModeBadBootCount);
        }
    }

    // Write it's a bad boot and we'll say it's good after a defined period of time.
    standardPreferences->putBool(prefBootSuccess, false);

    safeModeSafeTime = millis() + safeModeGoodBootAfterTime;
}

void StandardFeatures::manageSafeMode()
{
    if (!safeModeBootIsGood && safeModeBadBootCount < safeModeBadBootTrigger && millis() > safeModeSafeTime)
    {
        safeModeBootIsGood = true;
        standardPreferences->putBool(prefBootSuccess, true);
        Log.println("Booted safely");

        if (safeModeBadBootCount > 0)
        {
            standardPreferences->putUChar(prefBadBootCount, 0);
        }
    }
}

void StandardFeatures::enableWiFi(const char *wifiSSID, const char *wifiPassword, const char *deviceName)
{
    _wifiEnabled = true;
    _wifiSSID = wifiSSID;
    _wifiPassword = wifiPassword;
    _deviceName = deviceName;
    connectToNetwork();
}

void StandardFeatures::disableWiFi()
{
    if (_otaEnabled)
        disableOTA();

    if (_mqttEnabled)
        disableMQTT();
    
    _wifiEnabled = false;
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

void StandardFeatures::connectToNetwork()
{
    WiFi.mode(WIFI_STA);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(_deviceName);
    WiFi.begin(_wifiSSID, _wifiPassword);

    if (WiFi.waitForConnectResult() == WL_CONNECTED && wifiReconnectCount == 0)
        Log.println("Connected to WiFi");
}

void StandardFeatures::manageWiFi()
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

void StandardFeatures::enableMQTT(const uint8_t *mqttServer, const char *mqttUsername, const char *mqttPassword, const char *mqttDeviceName)
{
    if (!_wifiEnabled)
        return;

    _mqttEnabled = true;
    _mqttClient = new PubSubClient(espClient);
    _mqttServer = mqttServer;
    _mqttUsername = mqttUsername;
    _mqttPassword = mqttPassword;
    _mqttDeviceName = mqttDeviceName;

    _mqttClient->setBufferSize(4096);
    _mqttClient->setServer(_mqttServer, 1883);
    connectToMQTT();
}

void StandardFeatures::disableMQTT()
{
    _mqttEnabled = false;
    _mqttClient->disconnect();
    _mqttClient = NULL;
}

void StandardFeatures::setMqttOnConnectCallback(void (*callback)())
{
    if (!_mqttEnabled)
        return;

    _onMQTTConnectCallback = callback;
    if (_mqttClient != NULL && _mqttClient->connected())
        _onMQTTConnectCallback();
}

void StandardFeatures::connectToMQTT()
{
    Log.println("Connecting to MQTT");
    // Attempt to connect
    if (WiFi.isConnected() && _mqttClient->connect(_deviceName, _mqttUsername, _mqttPassword))
    {
        Log.println("Connected to MQTT");
        nextMqttConnectAttempt = 0;
        
        if (_onMQTTConnectCallback)
            _onMQTTConnectCallback();
    }
    else
    {
        Log.println("Failed to connect to MQTT");
        nextMqttConnectAttempt = millis() + mqttReconnectInterval;
    }
}

bool StandardFeatures::mqttPublish(const char* topic, const char* payload, boolean retained = false)
{
    if (_mqttEnabled && _mqttClient->connected())
    {
        return _mqttClient->publish(topic, payload, retained);
    }

    return false;
}

bool StandardFeatures::mqttSubscribe(const char* topic)
{
    if (_mqttEnabled && _mqttClient->connected())
    {
        return _mqttClient->subscribe(topic);
    }
    return false;
}

void StandardFeatures::setMqttCallback(std::function<void(char*, uint8_t*, unsigned int)> callback)
{
    if (_mqttEnabled && _mqttClient->connected())
    {
        _mqttClient->setCallback(callback);
    }
}

void StandardFeatures::sendTelegrafMetrics()
{
    if (millis() > nextMetricsUpdate)
    {
        nextMetricsUpdate = millis() + 30000;
        uint32_t uptime = esp_timer_get_time() / 1000000;

        char buffer[200];
        snprintf(buffer, sizeof(buffer),
            "status,device=%s uptime=%d,resetReason=%d,firmware=\"%s\",appVersion=\"%s\",memUsed=%ld,memTotal=%ld",
            _mqttDeviceName,
            uptime,
            esp_reset_reason(),
            esp_get_idf_version(),
            _appVersion,
            (ESP.getHeapSize()-ESP.getFreeHeap()),
            ESP.getHeapSize());
        _mqttClient->publish("telegraf/particle", buffer);
    }
}

void StandardFeatures::manageMQTT()
{
    if (_mqttClient->connected())
    {
        _mqttClient->loop();

        if (millis() > nextMetricsUpdate)
        {
            sendTelegrafMetrics();
            nextMetricsUpdate = millis() + 30000;
        }
    }
    else if (millis() > nextMqttConnectAttempt)
    {
        if (WiFi.isConnected())
            connectToMQTT();
    }
}

#ifdef DIAGNOSTIC_PIXEL
void StandardFeatures::setupDiagnosticPixel()
{   
    _diagnosticPixel->begin();
    _diagnosticPixel->setPixelColor(0, NEOPIXEL_BLUE);
    _diagnosticPixel->setBrightness(diagnosticPixelMaxBrightness);
    _diagnosticPixel->show();
}

/*
void setMaxDiagnosticPixelBrightness(uint8_t brightness)
{
    diagnosticPixelMaxBrightness = brightness;
    diagnosticPixelUpdateGap = 1500 / brightness;
}
*/

void StandardFeatures::manageDiagnosticPixel()
{
    if (millis() < nextDiagnosticPixelUpdate)
        return;

    if (_mqttClient != NULL && _mqttClient->connected())
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
    _diagnosticPixel->setPixelColor(0, currentDiagnosticPixelColor);
    _diagnosticPixel->setBrightness(diagnosticPixelBrightness);
    _diagnosticPixel->show();

    nextDiagnosticPixelUpdate = millis() + diagnosticPixelUpdateGap;
}

void StandardFeatures::setDiagnosticPixelColor(uint32_t color)
{
    diagnosticPixelColor2 = color;
}

void StandardFeatures::enableDiagnosticPixel(uint8_t pin)
{
    _diagnosticPixelEnabled = true;
    _diagnosticPixelPin = pin;
    _diagnosticPixel = new Adafruit_NeoPixel(1, _diagnosticPixelPin, NEO_GRB + NEO_KHZ800);
    setupDiagnosticPixel();
}
#else
void StandardFeatures::setupDiagnosticPixel() {}
void StandardFeatures::manageDiagnosticPixel() {}
void StandardFeatures::enableDiagnosticPixel(uint8_t pin) {}
void StandardFeatures::setDiagnosticPixelColor(uint32_t color) {}
#endif

#ifdef DIAGNOSTIC_LED
void StandardFeatures::enableDiagnosticLed(uint8_t ledPin)
{
    _diagnosticLedEnabled = true;
    _diagnosticLedPin = ledPin;
    pinMode(_diagnosticLedPin, OUTPUT);
    digitalWrite(_diagnosticLedPin, HIGH);
}

void StandardFeatures::manageDiagnosticLed()
{
    if (millis() < _nextDiagnosticLedUpdate)
        return;

    _nextDiagnosticLedUpdate = millis() + _diagnosticLedUpdateTime;
    _diagnosticLedState = !_diagnosticLedState;
    digitalWrite(_diagnosticLedPin, _diagnosticLedState ? HIGH : LOW);
}

void StandardFeatures::setDiagnosticLEDUpdateTime(uint16_t pause)
{
    _diagnosticLedUpdateTime = pause;
}

#else
void StandardFeatures::enableDiagnosticLed(uint8_t ledPin) {}
void StandardFeatures::manageDiagnosticLed() {}
void StandardFeatures::setDiagnosticLEDUpdateTime(uint16_t pause) {}
#endif

void StandardFeatures::enableOTA(const char *hostname, const char *otaPassword)
{
    if (!_wifiEnabled)
        return;

    _otaEnabled = true;
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPassword(otaPassword);

    ArduinoOTA.onStart([this]()
    {
        Log.println("OTA Start");
        otaRunning = true;
        #ifdef DIAGNOSTIC_PIXEL
        _diagnosticPixel->setPixelColor(0, NEOPIXEL_WHITE);
        _diagnosticPixel->setBrightness(diagnosticPixelMaxBrightness);
        _diagnosticPixel->show();
        #endif
    });

    ArduinoOTA.onEnd([this]()
    {
        Log.println("OTA End");
        otaRunning = false;
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
    });

    ArduinoOTA.onError([this](ota_error_t error)
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
        otaRunning = false;
    });
    
    ArduinoOTA.begin();
}

void StandardFeatures::disableOTA()
{
    _otaEnabled = false;
    ArduinoOTA.end();
}

void StandardFeatures::loop()
{
    if (_diagnosticPixelEnabled)
    {
        manageDiagnosticPixel();
    }

    if (_diagnosticLedEnabled)
    {
        manageDiagnosticLed();
    }

    if (_safeModeEnabled)
    {
        manageSafeMode();
    }

    if (_wifiEnabled)
    {
        manageWiFi();
    

        if (_mqttEnabled)
        {
            manageMQTT();
        }

        if (_otaEnabled)
        {
            ArduinoOTA.handle();
        }
    }
}

#endif // STANDARD_FEATURES_H