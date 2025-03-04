#include <Wire.h>
#include <SC16IS752.h>

SC16IS752 i2cuart(SC16IS750_PROTOCOL_I2C, SC16IS750_ADDRESS_AA);

#define BUFFER_SIZE 64  // Define buffer size

char receivedMessage[BUFFER_SIZE];  // Buffer for storing message
int bufferIndex = 0;  // Renamed from `index` to `bufferIndex`

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Receiver Starting...");

    i2cuart.begin(9600, 9600); 

    if (i2cuart.ping() != 1) {
        Serial.println("SC16IS752 not found!");
        while (1);
    } else {
        Serial.println("SC16IS752 found, ready to receive.");
    }
}

void loop() {
    while (i2cuart.available(SC16IS752_CHANNEL_A)) {
        char c = i2cuart.read(SC16IS752_CHANNEL_A);

        // If message exceeds buffer, reset index
        if (bufferIndex >= BUFFER_SIZE - 1) {
            bufferIndex = 0;
        }

        // Store character in buffer
        receivedMessage[bufferIndex++] = c;

        // If newline received, print full message
        if (c == '\n') {
            receivedMessage[bufferIndex - 1] = '\0';  // Replace newline with null terminator
            Serial.print("Received: ");
            Serial.println(receivedMessage);
            bufferIndex = 0;  // Reset buffer index for next message
        }
    }
}
