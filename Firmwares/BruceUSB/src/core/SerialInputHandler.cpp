#include "SerialInputHandler.h"
#include "Arduino.h"  // for Serial
#include "navflags.h"  // wherever the volatile flags are defined

void SerialInputHandler::pressUp() {
    Serial.println("[+] UP pressed");
    UpPress = true;
    AnyKeyPress = true;
}

void SerialInputHandler::pressDown() {
    Serial.println("[+] DOWN pressed");
    DownPress = true;
    AnyKeyPress = true;
}

void SerialInputHandler::pressLeft() {
    Serial.println("[+] LEFT pressed");
    PrevPress = true;
    AnyKeyPress = true;
}

void SerialInputHandler::pressRight() {
    Serial.println("[+] RIGHT pressed");
    NextPress = true;
    AnyKeyPress = true;
}

void SerialInputHandler::pressEnter() {
    Serial.println("[+] ENTER pressed");
    SelPress = true;
    AnyKeyPress = true;
}

void SerialInputHandler::pressEsc() {
    Serial.println("[+] Esc Pressed");
    EscPress = true;
    AnyKeyPress = true;
}

void SerialInputHandler::pressLong() {
    Serial.println("[+] Long Press");
    LongPress = true;
    AnyKeyPress = true;
}

SerialInputHandler serialInputHandler;  // definition (only one place)

