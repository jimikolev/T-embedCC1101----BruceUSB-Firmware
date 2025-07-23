#ifndef SERIAL_INPUT_HANDLER_H
#define SERIAL_INPUT_HANDLER_H

class SerialInputHandler {
public:
    void pressUp();
    void pressDown();
    void pressLeft();
    void pressRight();
    void pressEnter();
    void pressEsc();
    void pressLong();
};

extern SerialInputHandler serialInputHandler;

#endif // SERIAL_INPUT_HANDLER_H
