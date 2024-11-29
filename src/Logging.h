#ifndef __LOGGING_H_
#define __LOGGING_H_

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

    void enableLogging(const char *appName, const char *syslogServer, uint16_t sysLogPort = 514)
    {
        _appName = appName;
        _syslogServer = syslogServer;
        _syslogPort = sysLogPort;
        Serial.begin(9600);
    }

    void enableSerial(bool enable) { _enableSerial = enable; };
    void enableSyslog(bool enable) { _enableSyslog = enable; };

    size_t write(byte a)
    {
        if (_enableSyslog)
            return syslogwrite(a, _severity);

        if (_enableSerial)
            return Serial.write(a);

        return 1;
    };

private:
    bool _enableSerial = true;
    bool _enableSyslog = true;
    const uint8_t _defaultSeverity = 14;
    uint8_t _severity = _defaultSeverity;
    uint16_t _syslogPort = 514;
    const char * _syslogServer = "";
    size_t at = 0;
    char _macAddress[80] = "";
    const char * _appName = "";
    char logbuff[512]; // 1024 seems to be to large for some syslogd's.
    WiFiUDP syslog;
    bool _macAddressSet = false;

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

            if (WiFi.isConnected())
            {
                if (!_macAddressSet)
                {
                    String macString = WiFi.macAddress();
                    macString.toLowerCase();
                    sprintf(_macAddress, "%s", macString.c_str());
                    _macAddressSet = true;
                }

                syslog.beginPacket(_syslogServer, _syslogPort);
                syslog.printf("<%d>1 - %s %s - - - %s", s, _macAddress, _appName, logbuff);
                syslog.endPacket();
            }
        };
        return 1;
    };

};
extern TLog Log;
#endif