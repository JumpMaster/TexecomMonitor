// Copyright 2021 Kevin Cooper

#include "texecom.h"

Texecom::Texecom(void (*alarmCallback)(ALARM_STATE, uint8_t), void (*zoneCallback)(uint8_t, uint8_t))
{
    this->alarmCallback = alarmCallback;
    this->zoneCallback = zoneCallback;
}

void Texecom::setup()
{
    texSerial.begin(19200, SERIAL_8N2, RXD2, TXD2);  // open serial communications

    pinMode(pinFullArmed, INPUT);
    pinMode(pinPartArmed, INPUT);
    pinMode(pinEntry, INPUT);
    pinMode(pinExit, INPUT);
    pinMode(pinTriggered, INPUT);
    pinMode(pinArmFailed, INPUT);
    pinMode(pinFaultPresent, INPUT);
    pinMode(pinAreaReady, INPUT);
}

void Texecom::loop()
{
    if (millis() > nextPinCheck)
    {
        nextPinCheck = millis() + pinCheckFrequency;
        checkDigiOutputs();
    }
    checkSerial();

    if (lastAlarmStateChange != 0 && millis() > (lastAlarmStateChange + alarmStateChangeBuffer))
    {
        lastAlarmStateChange = 0;
        alarmState = newAlarmState;
        alarmCallback(alarmState, alarmStateFlags);
    }
}

void Texecom::checkDigiOutputs()
{
    bool changeDetected = false;
    bool _state = digitalRead(pinFullArmed);

    if (_state != statePinFullArmed)
    {
        statePinFullArmed = _state;
        if (_state == LOW)
        {
            // Log.info("Pin Full Armed");
            changeDetected = true;
            newAlarmState = ARMED_AWAY;
        }
    }

    _state = digitalRead(pinPartArmed);

    if (_state != statePinPartArmed)
    {
        statePinPartArmed = _state;
        if (_state == LOW)
        {
            // Log.info("Pin Part Armed");
            changeDetected = true;
            newAlarmState = ARMED_HOME;
        }
    }

    _state = digitalRead(pinEntry);

    if (_state != statePinEntry)
    {
        statePinEntry = _state;
        if (_state == LOW)
        {
            // Log.info("Pin Entry");
            changeDetected = true;
            newAlarmState = ENTRY;
        }
    }

    _state = digitalRead(pinExit);

    if (_state != statePinExit)
    {
        statePinExit = _state;
        if (_state == LOW)
        {
            // Log.info("Pin Exit");
            changeDetected = true;
            newAlarmState = EXIT;
        }
    }

    _state = digitalRead(pinTriggered);

    if (_state != statePinTriggered)
    {
        statePinTriggered = _state;
        if (_state == LOW)
        {
            // Log.info("Pin Triggered");
            changeDetected = true;
            newAlarmState = TRIGGERED;
        }
    }

    _state = digitalRead(pinAreaReady);

    if (_state != statePinAreaReady)
    {
        // Log.info("Pin Ready");
        changeDetected = true;
        statePinAreaReady = _state;

        if (statePinAreaReady == LOW)
        {
            alarmStateFlags |= ALARM_READY;
        }
        else
        {
            alarmStateFlags &= ~ALARM_READY;
        }
    }

    _state = digitalRead(pinFaultPresent);

    if (_state != statePinFaultPresent)
    {
        // Log.info("Pin Fault");
        changeDetected = true;
        statePinFaultPresent = _state;

        if (statePinFaultPresent == LOW)
        {
            alarmStateFlags |= ALARM_FAULT;
            Log.println("Alarm is reporting a fault");
        }
        else
        {
            alarmStateFlags &= ~ALARM_FAULT;
            Log.println("Alarm fault cleared");
        }
    }

    _state = digitalRead(pinArmFailed);

    if (_state != statePinArmFailed)
    {
        // Log.info("Pin Arm Failed");
        changeDetected = true;
        statePinArmFailed = _state;

        if (statePinArmFailed == LOW)
        {
            alarmStateFlags |= ALARM_ARM_FAILED;
            Log.println("Alarm failed to arm");
        }
        else
        {
            alarmStateFlags &= ~ALARM_ARM_FAILED;
            Log.println("Alarm arm failure cleared");
        }
    }


    // If no pins are high state must be disarmed
    if (newAlarmState != DISARMED &&
        statePinFullArmed == HIGH &&
        statePinPartArmed == HIGH &&
        statePinEntry == HIGH &&
        statePinExit == HIGH &&
        statePinTriggered == HIGH)
    {

            changeDetected = true;
            newAlarmState = DISARMED;
    }

    if (changeDetected)
    {
        lastAlarmStateChange = millis();
    }

}

void Texecom::decodeZoneState(char *message)
{
    uint8_t zone;
    uint8_t state;

    char zoneChar[4];
    memcpy(zoneChar, &message[2], 3);
    zoneChar[3] = '\0';
    zone = atoi(zoneChar) - firstZone;

    state = message[5] - '0';

    if (state == 0) // Healthy
    {
        zoneStates[zone] &= ~Texecom::ZONE_ACTIVE;
        zoneStates[zone] &= ~Texecom::ZONE_TAMPER;
    }
    else if (state == 1) // Active
    {
        zoneStates[zone] |= Texecom::ZONE_ACTIVE;
        zoneStates[zone] &= ~Texecom::ZONE_TAMPER;
    }
    else if (state == 2) // Tamper
    {
        zoneStates[zone] &= ~Texecom::ZONE_ACTIVE;
        zoneStates[zone] |= Texecom::ZONE_TAMPER;
    }

    zoneCallback(zone+firstZone, zoneStates[zone]);
}

bool Texecom::processCrestronMessage(char *message, uint8_t messageLength)
{
    // Zone state changed
    if (messageLength == 6 &&
        strncmp(message, msgZoneUpdate, strlen(msgZoneUpdate)) == 0)
    {
        decodeZoneState(message);
        return true;
    }
    // System Armed
    else if (messageLength >= 6 &&
             strncmp(message, msgArmUpdate, strlen(msgArmUpdate)) == 0)
    {
        return true;
    }
    // System Disarmed
    else if (messageLength >= 6 &&
             strncmp(message, msgDisarmUpdate, strlen(msgDisarmUpdate)) == 0)
    {
        return true;
    }
    // Entry while armed
    else if (messageLength == 6 &&
             strncmp(message, msgEntryUpdate, strlen(msgEntryUpdate)) == 0)
    {
        return true;
    }
    // System arming
    else if (messageLength == 6 &&
             strncmp(message, msgArmingUpdate, strlen(msgArmingUpdate)) == 0)
    {
        return true;
    }
    // Intruder
    else if (messageLength == 6 &&
             strncmp(message, msgIntruderUpdate, strlen(msgIntruderUpdate)) == 0)
    {
        return true;
    }
    // User logged in with code or tag
    else if (messageLength == 6 &&
            (strncmp(message, msgUserPinLogin, strlen(msgUserPinLogin)) == 0 ||
            strncmp(message, msgUserTagLogin, strlen(msgUserTagLogin)) == 0))
    {
        int user = message[4] - '0';
        if (user < userCount)
            Log.printf("User logged in: %s\n", users[user]);
        else
            Log.println("User logged in: Outside of user array size");
        return true;
    }
    // Reply to ASTATUS request that the system is disarmed
    else if (messageLength == 5 &&
             strncmp(message, msgReplyDisarmed, strlen(msgReplyDisarmed)) == 0)
    {
        return true;
    }
    // Reply to ASTATUS request that the system is armed
    else if (messageLength == 5 &&
             strncmp(message, msgReplyArmed, strlen(msgReplyArmed)) == 0) 
    {
        return true;
    }
    else if ((messageLength >= strlen(msgScreenArmedPart) &&
              strncmp(message, msgScreenArmedPart, strlen(msgScreenArmedPart)) == 0) ||
             (messageLength >= strlen(msgScreenArmedNight) &&
              strncmp(message, msgScreenArmedNight, strlen(msgScreenArmedNight)) == 0) ||
             (messageLength >= strlen(msgScreenIdlePartArmed) &&
              strncmp(message, msgScreenIdlePartArmed, strlen(msgScreenIdlePartArmed)) == 0))
    {
        return true;
    }
    else if (messageLength >= strlen(msgScreenArmedFull) &&
             strncmp(message, msgScreenArmedFull, strlen(msgScreenArmedFull)) == 0)
    {
        return true;
    }
    else if (messageLength >= strlen(msgScreenIdle) &&
             strncmp(message, msgScreenIdle, strlen(msgScreenIdle)) == 0)
    {
        return true;
    }
    // Shown directly after user logs in
    else if (messageLength > strlen(msgWelcomeBack) &&
             strncmp(message, msgWelcomeBack, strlen(msgWelcomeBack)) == 0)
    {
        return true;
    }
    // Shown shortly after user logs in
    else if (messageLength >= strlen(msgScreenQuestionArm) &&
             strncmp(message, msgScreenQuestionArm, strlen(msgScreenQuestionArm)) == 0)
    {
        return true;
    }
    else if (messageLength >= strlen(msgScreenQuestionPartArm) &&
             strncmp(message, msgScreenQuestionPartArm, strlen(msgScreenQuestionPartArm)) == 0)
    {
        return true;
    }
    else if (messageLength >= strlen(msgScreenQuestionNightArm) &&
             strncmp(message, msgScreenQuestionNightArm, strlen(msgScreenQuestionNightArm)) == 0)
    {
        return true;
    }
    else if (messageLength >= strlen(msgScreenQuestionDisarm) &&
             strncmp(message, msgScreenQuestionDisarm, strlen(msgScreenQuestionDisarm)) == 0)
    {
        return true;
    }
    else if (messageLength >= strlen(msgScreenAreainEntry) &&
             strncmp(message, msgScreenAreainEntry, strlen(msgScreenAreainEntry)) == 0)
    {
        return true;
    }
    else if (messageLength >= strlen(msgScreenAreainExit) &&
             strncmp(message, msgScreenAreainExit, strlen(msgScreenAreainExit)) == 0)
    {
        return true;
    }
    return false;
}

void Texecom::checkSerial()
{
    bool messageReady = false;
    uint8_t messageLength = 0;

    // Read incoming serial data if available and copy to TCP port
    while (texSerial.available() > 0)
    {
        int incomingByte = texSerial.read();
        // Log.info("S %d", incomingByte);
        if (bufferPosition == 0)
        {
            messageStart = millis();
        }

        // Will never happen but just in case
        if (bufferPosition >= maxMessageSize)
        {
            memcpy(message, buffer, bufferPosition);
            message[bufferPosition] = '\0';
            messageReady = true;
            messageLength = bufferPosition;
            bufferPosition = 0;
            break;
        // 13+10 (CRLF) signifies the end of message
        }
        else if (bufferPosition > 2 &&
                 incomingByte == 10 &&
                 buffer[bufferPosition-1] == 13)
        {

            buffer[bufferPosition-1] = '\0'; // Replace 13 with termination
            memcpy(message, buffer, bufferPosition);
            messageReady = true;
            messageLength = bufferPosition-1;

            if (messageReady)
            {
                bufferPosition = 0;
                break;
            }

        }
        else
        {
            buffer[bufferPosition++] = incomingByte;
        }
    } // while (texSerial.available() > 0)

    if (bufferPosition > 0 && millis() > (messageStart+50))
    {
        Log.println("Message failed to receive within 50ms");
        memcpy(message, buffer, bufferPosition);
        message[bufferPosition] = '\0';
        messageReady = true;
        messageLength = bufferPosition;
        bufferPosition = 0;
    }

    if (messageReady)
    {
        Log.println(message);

        bool processedSuccessfully = false;
        processedSuccessfully = processCrestronMessage(message, messageLength);

        if (!processedSuccessfully)
        {
            if (message[0] == '"')
            {
                Log.printf("Unknown Crestron command - %s\n", message);
            }
            else
            {
                Log.printf("Unknown non-Crestron command - %s\n", message);
                for (uint8_t i = 0; i < messageLength; i++)
                {
                    Log.printf("%d\n", message[i]);
                }
            }
        }
    }
}