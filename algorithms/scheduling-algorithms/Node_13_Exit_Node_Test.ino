// Include necessary libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// UART Configurations
#define UART_1_TX_PIN 26         // Pins Left Side : Physical Pin 11
#define UART_1_RX_PIN 27         // Pins Left Side : Physical Pin 12

#define BAUD_RATE 115200   // Common baud rate for UART communication
#define BAUD_RATE_UART 9600   // Common baud rate for UART communication

// Unique identifier for this ESP32 node
#define NODE_ID 13  // Change this for each ESP32 node in the network

// FreeRTOS queue handle for managing incoming data packets
QueueHandle_t dataQueue;

// Task handle (Optional?)
TaskHandle_t sendTaskHandle;
TaskHandle_t sendDataHandle;
TaskHandle_t processTaskHandle;

// Structure to hold incoming data and where to send it
struct DataPacket {
    String data;
    String direction;
};

// Function to initialize UART
void setupUART(){
  Serial.begin(BAUD_RATE);
  Serial1.begin(BAUD_RATE_UART, SERIAL_8N1, UART_1_TX_PIN, UART_1_RX_PIN); // Left Side
}

// Function to send data only to the left UART
void sendData(String jsonString, int Dest_Node_ID) {
  if (Dest_Node_ID < NODE_ID) { // Only send to the left
    Serial1.println(jsonString);
    Serial.print("[Send Task] Sent to ");
    Serial.print(Dest_Node_ID);
    Serial.print(": ");
    Serial.println(jsonString);
  }
}

void pathingAlgorithm(){}

// UART Listener Task for receiving data from the Left UART
void uartListenerTask(void *parameter) {
    char incomingData[200];  // Buffer to store incoming data

    while (1) {
        // Clear buffer to avoid residual data issues
        memset(incomingData, 0, sizeof(incomingData));

        // Check if left UART has available data
        if (Serial1.available()) {
            size_t len = Serial1.readBytesUntil('\n', incomingData, sizeof(incomingData) - 1);
            incomingData[len] = '\0'; // Null-terminate the string

            // Create a data packet and push it to the queue
            DataPacket packet = {String(incomingData), "left"};
            xQueueSend(dataQueue, &packet, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to avoid busy-waiting
    }
}

// Central Processing Task for handling incoming data from the queue
void processTask(void *parameter) {
    DataPacket receivedPacket;
    StaticJsonDocument<200> doc;  // JSON document for parsing

    while (1) {
        // Wait to receive a data packet from the queue
        if (xQueueReceive(dataQueue, &receivedPacket, portMAX_DELAY) == pdTRUE) {
            // Attempt to parse the JSON data
            DeserializationError error = deserializeJson(doc, receivedPacket.data);

            if (!error) {
                String payload = doc["Payload"];  // Extract the payload data from JSON
                int node_ID = doc["Node_Id"];  // Extract the destination ID from JSON
                String status = doc["Status"];

                if ((node_ID == NODE_ID) && (status == "False")) {
                    // If the data is meant for this node, process it
                    Serial.print("[Process Task] Processing Data: ");
                    Serial.println(payload);

                    // Modify the payload and set the next destination (example logic)
                    doc["Payload"] = payload + " Processed Data : ";
                    doc["Node_Id"] = 13;  // Hardcoded destination as the exit node
                    doc["Status"] = "True";

                    // Serialize the modified JSON and forward (though this node is the exit)
                    String jsonString;
                    serializeJson(doc, jsonString);
                    Serial.print("[Process Task] Sending Modified Data to Server: ");
                    Serial.println(jsonString);
                } 
                else if((node_ID == NODE_ID) && (status == "True")){
                    Serial.print("This packet as entered the Exit Node as Completed -----Shipping: ");
                    Serial.println(payload);
                }
                else {
                    // If not for this node, forward the data to the left
                    sendData(receivedPacket.data.c_str(), doc["Node_Id"]);
                }
            } else {
                Serial.println("[Process Task] Failed to parse JSON");
            }
        }
    }
}

void setup() {
    setupUART(); // Initialize UARTs
    Serial.println("ESP32 UART Communication with FreeRTOS Started");

    // Create a FreeRTOS queue with capacity for 10 DataPacket items
    dataQueue = xQueueCreate(10, sizeof(DataPacket));

    // Create UART listener task for the left UART
    xTaskCreate(uartListenerTask, "UART Left Listener", 2048, NULL, 1, NULL);
    
    // Create the central processing task
    xTaskCreate(processTask, "Process Task", 4096, NULL, 2, NULL);
}

void loop() {
    // The loop remains empty because FreeRTOS handles task scheduling
}
