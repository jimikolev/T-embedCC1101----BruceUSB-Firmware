#ifndef NAVFLAGS_H
#define NAVFLAGS_H

#include <Arduino.h>

// Shared input state flags
extern volatile bool NextPress;
extern volatile bool PrevPress;
extern volatile bool UpPress;
extern volatile bool DownPress;
extern volatile bool SelPress;
extern volatile bool EscPress;
extern volatile bool AnyKeyPress;
extern volatile bool NextPagePress;
extern volatile bool PrevPagePress;
extern volatile bool LongPress;
extern volatile bool SerialCmdPress;
extern volatile int forceMenuOption;
extern volatile uint8_t menuOptionType;
extern String menuOptionLabel;

extern bool isSleeping;
extern bool isScreenOff;
extern bool gpsConnected;
extern bool isWebUIActive;
extern bool returnToMenu;


#endif
