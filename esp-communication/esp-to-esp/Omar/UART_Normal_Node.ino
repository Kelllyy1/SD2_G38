#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TX_Pin_1 17 // Send to the Right (Physical Pin 28)
#define RX_Pin_1 16 // Receive from the Right (Physical Pin 27)

#define TX_Pin_2 25 // Send to the Left (Physical Pin 9)
#define RX_Pin_2 26 // Receive from the Left (Physical Pin 10)

char label = '2';

void uartReceiveTask(void *parameter) {
    while (1) {
        if (Serial1.available()) {
            String receivedMessage = Serial1.readStringUntil('\n');
            Serial1.println("ACK");  // Send ACK after processing
            Serial.println("Received: " + receivedMessage);

            String data_message = checkMessage(receivedMessage, 0);
            String node_num = checkMessage(receivedMessage, 1);
            String data_status = checkMessage(receivedMessage, 2);

            if (node_num[0] == label) {
                Serial.println("Processing Data ----");
                String processedString = data_message + ":" + node_num + ":C";
                Serial.println("Processed: " + processedString);
                Serial2.println(processedString);
            } else if (data_status == "C") {
                Serial.println("*** Data Complete *** Forwarding to Exit Node ***");
                Serial2.println(receivedMessage);
            } else {
                Serial.println("Forwarding to the right: " + receivedMessage);
                Serial2.println(receivedMessage);
            }

            Serial.println("--------------------------------------");
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, TX_Pin_2, RX_Pin_2);
    Serial2.begin(9600, SERIAL_8N1, TX_Pin_1, RX_Pin_1);

    xTaskCreate(uartReceiveTask, "UART Receive Task", 2048, NULL, 1, NULL);
}

void loop() {
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

// Function to parse messages based on ':'
String checkMessage(String data, int index) {
    int start = 0, count = 0;
    char delimiter = ':';
    int end = data.indexOf(delimiter);

    while (end != -1) {
        if (count == index) return data.substring(start, end);
        start = end + 1;
        end = data.indexOf(delimiter, start);
        count++;
    }

    return count == index ? data.substring(start) : "";
}
