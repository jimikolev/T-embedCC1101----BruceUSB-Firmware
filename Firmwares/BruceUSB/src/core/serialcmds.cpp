#include "serialcmds.h"
#include "utils.h"
#include <globals.h>
#include "SerialInputHandler.h"
extern void InputHandler();

void handleSerialCommands() {
    if (!Serial.available()) return;

    String cmd_str = Serial.readStringUntil('\n');

    // Handle key commands from the GUI
    if (cmd_str.startsWith("[KEY:")) {
        if (cmd_str.indexOf("UP") >= 0) {
            serialInputHandler.pressUp();
        } else if (cmd_str.indexOf("DOWN") >= 0) {
            serialInputHandler.pressDown();
        } else if (cmd_str.indexOf("LEFT") >= 0) {
            serialInputHandler.pressLeft();
        } else if (cmd_str.indexOf("RIGHT") >= 0) {
            serialInputHandler.pressRight();
        } else if (cmd_str.indexOf("ENTER") >= 0) {
            serialInputHandler.pressEnter();
        } else if (cmd_str.indexOf("ESC") >= 0) {
            serialInputHandler.pressEsc();
        } else if (cmd_str.indexOf("LONG") >= 0) {
            serialInputHandler.pressLong();
        }
        InputHandler();     // 👈 This actually processes the press flags
        Serial.print("$ "); // prompt
        backToMenu();       // refresh menu after GUI input
        return;
    }

    // Handle normal CLI commands
    serialCli.parse(cmd_str);
    Serial.print("$ "); // prompt
    backToMenu();       // forced menu redraw
}

void _serialCmdsTaskLoop(void *pvParameters) {
    Serial.begin(115200);

    while (1) {
        handleSerialCommands();
        vTaskDelay(500);
    }
}

void startSerialCommandsHandlerTask() {
    TaskHandle_t serialcmdsTaskHandle;

    xTaskCreatePinnedToCore(
        _serialCmdsTaskLoop, // Function to implement the task
        "serialcmds",        // Name of the task (any string)
        20000,               // Stack size in bytes
        NULL, // This is a pointer to the parameter that will be passed to the new task. We are not using it
              // here and therefore it is set to NULL.
        2,                     // Priority of the task
        &serialcmdsTaskHandle, // Task handle (optional, can be NULL).
        1 // Core where the task should run. By default, all your Arduino code runs on Core 1 and the Wi-Fi
          // and RF functions
    ); // (these are usually hidden from the Arduino environment) use the Core 0.
}
