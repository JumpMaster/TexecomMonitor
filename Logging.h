#ifndef __LOGGING_H_
#define __LOGGING_H_

#define SYSLOG_SERVER   "lb.home.cooper.uk";
#define SYSLOG_PORT     514
#define APP_NAME        "TexecomMonitor-ESP32"

#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#endif

/*
    User-Level Severity
	Emergency   Alert       Critical    Error       Warning     Notice      Info        Debug
    8           9           10          11          12          13          14          15
*/

class TLog : public Print
{
public:
    void printlnCritical(const char * c) { _severity = 10; this->println(c); _severity = _defaultSeverity;}
    void printlnError(const char * c) { _severity = 11; this->println(c); _severity = _defaultSeverity;}
    void printlnWarning(const char * c) { _severity = 10; this->println(c); _severity = _defaultSeverity;}

    void setup()
    {
        Serial.begin(115200);
        
        #ifdef ESP32
        uint8_t chipid[6];
        esp_read_mac(chipid, ESP_MAC_WIFI_STA);
        sprintf(_macAddress, "%02x:%02x:%02x:%02x:%02x:%02x",chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
        #else
        #endif
    }
    void disableSerial(bool onoff) { _disableSerial = onoff; };
    void disableSyslog(bool onoff) { _disableSyslog = onoff; };

    size_t write(byte a)
    {
      if (!_disableSyslog)
        syslogwrite(a, _severity);

      if (!_disableSerial)
        return Serial.write(a);
      
      return 1;
    };
private:
    bool _disableSerial = false;
    bool _disableSyslog = false;
    const uint8_t _defaultSeverity = 14;
    uint8_t _severity = _defaultSeverity;
    uint16_t _syslogPort = SYSLOG_PORT;
    const char * _syslogDest = SYSLOG_SERVER;
    size_t at = 0;
    char _macAddress[80] = "RUN-SETUP";
    const char * _appName = APP_NAME;
    char logbuff[512]; // 1024 seems to be to large for some syslogd's.
    WiFiUDP syslog;

    size_t syslogwrite(uint8_t c, uint8_t s = 14)
    {
        if (at >= sizeof(logbuff) - 1)
        {
            this->println("Purged logbuffer (should never happen)");
            at = 0;
        };

        if (c >= 32 && c < 128)
            logbuff[ at++ ] = c;

        if (c == '\n' || at >= sizeof(logbuff) - 1)
        {

            logbuff[at++] = 0;
            at = 0;

            syslog.beginPacket(_syslogDest, _syslogPort);
            syslog.printf("<%d>1 - %s %s - - - %s", s, _macAddress, _appName, logbuff);
            syslog.endPacket();
        };
        return 1;
    };

};
extern TLog Log;
#endif