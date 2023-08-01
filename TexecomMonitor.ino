#include "StandardFeatures.h"
#include "TexecomMonitor.h"

// Stubs
void mqttCallback(char* topic, byte* payload, unsigned int length);
void zoneCallback(uint8_t zone, uint8_t state);
void alarmCallback(Texecom::ALARM_STATE state, uint8_t flags);

Texecom texecom(alarmCallback, zoneCallback);
/*
MQTT mqttClient(mqttServer, 1883, mqttCallback);
uint32_t lastMqttConnectAttempt;
const int mqttConnectAtemptTimeout1 = 5000;
const int mqttConnectAtemptTimeout2 = 30000;
unsigned int mqttConnectionAttempts;
bool mqttStateConfirmed = true;

uint32_t resetTime = 0;
retained uint32_t lastHardResetTime;
retained int resetCount;

#define WATCHDOG_TIMEOUT_MS 30*1000

#define WDT_RREN_REG 0x40010508
#define WDT_CRV_REG 0x40010504
#define WDT_REG 0x40010000
void WatchDoginitialize() {
    *(uint32_t *) WDT_RREN_REG = 0x00000001;
    *(uint32_t *) WDT_CRV_REG = (uint32_t) (WATCHDOG_TIMEOUT_MS * 32.768);
    *(uint32_t *) WDT_REG = 0x00000001;
}

#define WDT_RR0_REG 0x40010600
#define WDT_RELOAD 0x6E524635
void WatchDogpet() {
    *(uint32_t *) WDT_RR0_REG = WDT_RELOAD;
}

PapertrailLogHandler papertrailHandler(papertrailAddress, papertrailPort,
  "ArgonTexecom", System.deviceID(),
  LOG_LEVEL_NONE, {
  { "app", LOG_LEVEL_ALL }
  // TOO MUCH!!! { “system”, LOG_LEVEL_ALL },
  // TOO MUCH!!! { “comm”, LOG_LEVEL_ALL }
});

void connectToMQTT() {
    lastMqttConnectAttempt = millis();
    bool mqttConnected = mqttClient.connect(System.deviceID(), mqttUsername, mqttPassword);
    if (mqttConnected) {
        mqttConnectionAttempts = 0;
        Log.info("MQTT Connected");
        mqttClient.subscribe("utilities/#");
    } else {
        mqttConnectionAttempts++;
        Log.info("MQTT failed to connect");
    }
}
*/
/*
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = '\0';

    if (strcmp(topic, "utilities/isDST") == 0) {
        if (strcmp(p, "true") == 0)
            Time.beginDST();
        else
            Time.endDST();
        
        if (Time.isDST())
            Log.info("DST is active");
        else
            Log.info("DST is inactive");
    }
}
*/
/*
uint32_t nextMetricsUpdate = 0;
void sendTelegrafMetrics() {
    if (millis() > nextMetricsUpdate) {
        nextMetricsUpdate = millis() + 30000;

        char buffer[150];
        snprintf(buffer, sizeof(buffer),
            "status,device=Texecom uptime=%d,resetReason=%d,firmware=\"%s\",memTotal=%ld,memUsed=%ld,ipv4=\"%s\"",
            System.uptime(),
            System.resetReason(),
            System.version().c_str(),
            DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_TOTAL_RAM),
            DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_USED_RAM),
            WiFi.localIP().toString().c_str()
            );
        mqttClient.publish("telegraf/particle", buffer);
    }
}
*/
void zoneCallback(uint8_t zone, uint8_t state) {

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

    if (mqttClient.connected()) {
        mqttClient.publish(attributesTopic, attributesMsg, true);
    }
}

void alarmCallback(Texecom::ALARM_STATE state, uint8_t flags) {

    Log.printf("Alarm: %s\n", alarmStateStrings[state]);

    char message[58];

    snprintf(message,
                sizeof(message),
                "{\"state\":\"%s\",\"ready\":%d,\"fault\":%d,\"arm_failed\":%d}",
                alarmStateStrings[state],
                (flags & Texecom::ALARM_READY) != 0,
                (flags & Texecom::ALARM_FAULT) != 0,
                (flags & Texecom::ALARM_ARM_FAILED) != 0);


    mqttClient.publish("home/security/alarm", message, true);
}
/*
void random_seed_from_cloud(unsigned seed) {
    srand(seed);
}

SYSTEM_THREAD(ENABLED)

void startupMacro() {
    System.enableFeature(FEATURE_RESET_INFO);
    System.enableFeature(FEATURE_RETAINED_MEMORY);
}
STARTUP(startupMacro());
*/
void setup() {

    StandardSetup();
    texecom.setup();
/*
    waitFor(Particle.connected, 30000);
    
    do {
        resetTime = Time.now();
        Particle.process();
    } while (resetTime < 1500000000 || millis() < 10000);
    
    if (System.resetReason() == RESET_REASON_PANIC) {
        if ((Time.now() - lastHardResetTime) < 120) {
            resetCount++;
        } else {
            resetCount = 1;
        }

        lastHardResetTime = Time.now();

        if (resetCount > 3) {
            System.enterSafeMode();
        }
    } else if (System.resetReason() == RESET_REASON_WATCHDOG) {
      Log.info("RESET BY WATCHDOG");
    } else {
        resetCount = 0;
    }

    Particle.publishVitals(900);

    WatchDoginitialize();
*/
    Log.println("Setup complete");//. Reset count = %d", resetCount);
/*
    connectToMQTT();

    uint32_t resetReasonData = System.resetReasonData();
    Particle.publish("pushover", String::format("ArgonAlarm: I am awake!: %d-%d", System.resetReason(), resetReasonData), PRIVATE);
*/
}

void loop()
{
/*
    if (mqttClient.isConnected()) {
        mqttClient.loop();
        sendTelegrafMetrics();
    } else if ((mqttConnectionAttempts < 5 && millis() > (lastMqttConnectAttempt + mqttConnectAtemptTimeout1)) ||
                 millis() > (lastMqttConnectAttempt + mqttConnectAtemptTimeout2)) {
        connectToMQTT();
    }
*/
    StandardLoop();
    texecom.loop();

//    WatchDogpet();
}