#include <Wire.h>
#include <SC16IS752.h>

SC16IS752 i2cuart(SC16IS750_PROTOCOL_I2C, SC16IS750_ADDRESS_AA);

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Transmitter Starting...");
    
    // Initialize UART Extender
    i2cuart.begin(9600, 9600); 

    if (i2cuart.ping() != 1) {
        Serial.println("SC16IS752 not found!");
        while (1);
    } else {
        Serial.println("SC16IS752 found, ready to transmit.");
    }
}

void loop() {
    const char* message = "Hello from ESP32 #1";
    
    Serial.print("Sending: ");
    Serial.println(message);

    for (int i = 0; message[i] != '\0'; i++) {
        i2cuart.write(SC16IS752_CHANNEL_A, message[i]);
    }
    i2cuart.write(SC16IS752_CHANNEL_A, '\n');  // End message

    delay(1000);  // Send every second
}
