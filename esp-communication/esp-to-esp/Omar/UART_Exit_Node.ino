#include <Arduino.h>

// Define UART Pins
#define TX_Pin_1 17 // Send to the Right         Physical Pin 28
#define RX_Pin_1 16 // Receive from the Right    Physical Pin 27
#define TX_Pin_2 25 // Send to Left              Physical Pin 9
#define RX_Pin_2 26 // Receive from Left         Physical Pin 10

char label = 'T';

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, TX_Pin_2, RX_Pin_2);
    Serial2.begin(9600, SERIAL_8N1, TX_Pin_1, RX_Pin_1);
}

void loop() {
    if (Serial1.available()) { // Checking from the left side
        String receivedMessage = Serial1.readStringUntil('\n');
        receivedMessage.trim();  // Remove extra spaces and newlines
        Serial.println("Received: " + receivedMessage);

        String data_message = checkMessage(receivedMessage, 0);
        String node_num = checkMessage(receivedMessage, 1);
        String data_status = checkMessage(receivedMessage, 2);

        if (data_status == "C") {  // Check if data is complete
            Serial.println("Data from node " + node_num + " is complete. Sending to server...");
            Serial2.println("ACK");  // Send acknowledgment
        } else {
            Serial.println("Data is not complete, forwarding...");
            Serial2.println(receivedMessage);
        }

        Serial.println("--------------------------------------");
    }
}

// Function to parse message components
String checkMessage(String data, int index) {
    int start = 0;
    char delimiter = ':';
    int end = data.indexOf(delimiter);
    int count = 0;

    while (end != -1) {  
        if (count == index) {
            String result = data.substring(start, end); // Store in a variable
            result.trim(); // Trim the extracted substring
            return result;
        }
        start = end + 1;  
        end = data.indexOf(delimiter, start);  
        count++;  
    }

    if (count == index) {
        String result = data.substring(start); // Store in a variable
        result.trim(); // Trim the extracted substring
        return result;
    }
    
    return "";  // If index is out of range, return an empty string
}

