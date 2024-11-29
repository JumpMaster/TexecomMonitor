#ifndef TEXECOM_MONITOR_H
#define TEXECOM_MONITOR_H

#include "StandardFeatures.h"
#include "secrets.h"
#include "texecom.h"

StandardFeatures standardFeatures;

const char *alarmStateStrings[6] = {"disarmed", "armed_home", "armed_away", "pending", "pending", "triggered"};

#endif