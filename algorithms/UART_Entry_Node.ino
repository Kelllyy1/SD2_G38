#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TX_Pin 17  // Send to the Right (Physical Pin 28)
#define RX_Pin 16  // Receive from the Right (Physical Pin 27)

TaskHandle_t uartTaskHandle;

void uartSendTask(void *parameter) {
    String message = "Task:2:P";

    while (1) {
        Serial.println("Sending: " + message);
        Serial2.println(message);
        
        unsigned long startTime = millis();
        bool ackReceived = false;

        while (millis() - startTime < 100) { // Wait for ACK
            if (Serial2.available()) {
                String response = Serial2.readStringUntil('\n');
                if (response == "ACK") {
                    Serial.println("ACK Received!");
                    ackReceived = true;
                    break;
                }
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Allow other tasks to run
        }

        if (!ackReceived) {
            Serial.println("ACK not received, resending...");
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);  // Avoid flooding
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, TX_Pin, RX_Pin);

    xTaskCreate(uartSendTask, "UART Send Task", 2048, NULL, 1, &uartTaskHandle);
}

void loop() {
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
