# PCA9554 library for Aruino

This library was created for use with NXP9554 8bit I/O expander. 
Please note, this expander has an open-drain, active-low INT output if the state of input ports changes. This interrupt is cleared when the input port register is read.

## Example usage
```cpp
#include <Wire.h>
#include <PCA9554.h>

PCA9554 ioexp(0x38, &Wire);
uint8_t input;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    ioexp.clearOutputs();                               // clear output register before setting pins to output mode
    ioexp.configMultiplePorts(PCA9554_OUTPUT, 0x0F);    // setting pins 0-3 as output
    ioexp.configMultiplePorts(PCA9554_INPUT, 0xF0);     // setting pins 4-7 as inputs
    ioexp.invertMultiplePorts(PCA9554_NO_INVERT);       // disabling input bit inversion on all ports
    ioexp.readInputs(&input);                           // reading all ports
    Serial.print("reading input register: 0x");
    Serial.println(input, HEX);
    delay(1000);
    ioexp.writeSinglePort(PCA9554_HIGH, PORT0);         // setting pin 0 high
    delay(1000);
    ioexp.writeMultiplePorts(PCA9554_HIGH, 0x0F);       // setting pins 0-3 high
    delay(1000);
    ioexp.writeMultiplePorts(PCA9554_LOW, 0x0F);        // setting pins 0-3 low
    delay(1000);
    ioexp.readInputs(&input, 0xF0);                     // reading only pins 4-7 (pins 0-3 are masked)
    Serial.print("reading pins 4-7: 0x");
    Serial.println(input, HEX);
    delay(1000);
    ioexp.invertMultiplePorts(PCA9554_INVERT, 0xF0);    // invert output value for ports 4-7 (pins 0-3 are masked)
    ioexp.readInputs(&input, 0xF0);                     // reading only pins 4-7 after inverting
    Serial.print("reading pins 4-7 after inverting: 0x");
    Serial.println(input, HEX);
}

void loop() {

}
```