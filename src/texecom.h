// Copyright 2021 Kevin Cooper

#ifndef __TEXECOM_H_
#define __TEXECOM_H_

#include "Arduino.h"
#include "Logging.h"

#define texSerial Serial2
#define RXD2 16
#define TXD2 17

#define firstZone 9 // Zone 1 = 1
#define zoneCount 11 // 1 == 1

class Texecom {
public:
  typedef enum {
      DISARMED = 0,
      ARMED_HOME = 1,
      ARMED_AWAY = 2,
      ENTRY = 3,
      EXIT = 4,
      TRIGGERED = 5,
  } ALARM_STATE;
  
  typedef enum {
      ZONE_ACTIVE = 1 << 0,
      ZONE_TAMPER = 1 << 1,
      ZONE_FAULT = 1 << 2,
      ZONE_FAILED_TEST = 1 << 3,
      ZONE_ALARMED = 1 << 4,
      ZONE_MANUAL_BYPASS = 1 << 5,
      ZONE_AUTO_BYPASS = 1 << 6,
      ZONE_ALWAYS_ZERO = 1 << 7,
  } ZONE_FLAGS;
  
  typedef enum {
      ALARM_READY = 1 << 0,
      ALARM_FAULT = 1 << 1,
      ALARM_ARM_FAILED = 1 << 2,
  } ALARM_FLAGS;

  Texecom(void (*alarmCallback)(Texecom::ALARM_STATE, uint8_t), void (*zoneCallback)(uint8_t, uint8_t));
  void setup();
  void loop();

private:

  void (*zoneCallback)(uint8_t, uint8_t);
  void (*alarmCallback)(Texecom::ALARM_STATE, uint8_t);

  uint8_t zoneStates[zoneCount];
  ALARM_STATE alarmState;
  ALARM_STATE newAlarmState;
  uint32_t lastAlarmStateChange = 0;
  const uint16_t alarmStateChangeBuffer = 1000;
  uint8_t alarmStateFlags;

  static const uint8_t userCount = 4;
  const char *users[userCount] = {"root", "Kevin", "Nicki", "Mumma"};

  const uint8_t maxMessageSize = 100;
  char message[101];
  char buffer[101];
  uint8_t bufferPosition;
  uint32_t messageStart;

  const uint16_t pinCheckFrequency = 1000;
  uint32_t nextPinCheck = 0;

//  Digi Output - Argon Pin - Texecom Configuration
//  1 ----------------- 18 - 22 Full Armed
//  2 ----------------- 39 - 23 Part Armed
//  3 ----------------- 5  - 19 Exit
//  4 ----------------- 34 - 17 Entry
//  5 ----------------- 4  - 00 Alarm
//  6 ----------------- 25 - 27 Arm Failed
//  7 ----------------- 36 - 66 Fault Present
//  8 ----------------- 26 - 16 Area Ready

  const int pinFullArmed = 18;
  const int pinPartArmed = 39;
  const int pinExit = 5;
  const int pinEntry = 34;
  const int pinTriggered = 4;
  const int pinArmFailed = 25;
  const int pinFaultPresent = 36;
  const int pinAreaReady = 26;

  bool statePinFullArmed = HIGH;
  bool statePinPartArmed = HIGH;
  bool statePinEntry = HIGH;
  bool statePinExit = HIGH;
  bool statePinTriggered = HIGH;
  bool statePinArmFailed = HIGH;
  bool statePinFaultPresent = HIGH;
  bool statePinAreaReady = LOW;

  const char *msgZoneUpdate = "\"Z0";
  const char *msgArmUpdate = "\"A0";
  const char *msgDisarmUpdate = "\"D0";
  const char *msgEntryUpdate = "\"E0";
  const char *msgArmingUpdate = "\"X0";
  const char *msgIntruderUpdate = "\"L0";
  const char *msgUserPinLogin = "\"U0";
  const char *msgUserTagLogin = "\"T0";

  const char *msgReplyDisarmed = "\"N";
  const char *msgReplyArmed = "\"Y";
  const char *msgWelcomeBack = "\"  Welcome Back";
  const char *msgScreenIdle = "\"  The Cooper's";
  const char *msgScreenIdlePartArmed = "\" * PART ARMED *";
  const char *msgScreenArmedPart = "\"Part";
  const char *msgScreenArmedNight = "\"Night";
  const char *msgScreenArmedFull = "\"Area FULL ARMED";

  const char *msgScreenQuestionArm = "\"Do you want to  Arm System?";
  const char *msgScreenQuestionPartArm = "\"Do you want to  Part Arm System?";
  const char *msgScreenQuestionNightArm = "\"Do you want:-   Night Arm";
  const char *msgScreenQuestionDisarm = "\"Do you want to  Disarm System?";

  const char *msgScreenAreainEntry = "\"Area in Entry";
  const char *msgScreenAreainExit = "\"Area in Exit >";

  void checkDigiOutputs();
  bool processCrestronMessage(char *message, uint8_t messageLength);
  void checkSerial();
  void decodeZoneState(char *message);
};

#endif  // __TEXECOM_H_