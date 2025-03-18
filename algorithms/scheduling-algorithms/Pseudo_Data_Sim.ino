// Include necessary libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// UART configuration
#define UART_2_TX_PIN 17         // Pins Right Side : Physical Pin 28
#define UART_2_RX_PIN 16         // Pins Right Side : Physical Pin 27

#define BAUD_RATE 115200
#define BAUD_RATE_UART 9600



//Structure to hold incoming data and where to send it
struct DataPacket {
    String data;
    int dest_node_id;
};

void setupUART() {
    Serial.begin(BAUD_RATE);           // Initialize Serial Monitor
    Serial2.begin(BAUD_RATE_UART, SERIAL_8N1, UART_2_TX_PIN, UART_2_RX_PIN);  // UART2 for communication
}


String jsonString_1;
String jsonString_5;
String jsonString_9;
String jsonString_13;

StaticJsonDocument<200> doc_1;
StaticJsonDocument<200> doc_5;
StaticJsonDocument<200> doc_9;
StaticJsonDocument<200> doc_13;




// Function to send data to a specific UART direction
void sendData(String jsonString, int node_id) {

    Serial2.println(jsonString);
    Serial.print("[Send Task] Sent to ");
    Serial.print(node_id);
    Serial.print(": ");
    Serial.println(jsonString);
  
}

void setup() {
  setupUART();  // Initialized UARTs
  Serial.println("ESP32 UART Communication with FreeRTOS Started: [Pseudo Data Sim]");

  doc_1["Payload"] = "This is DATA for a Specific Node [1]";
  doc_1["Node_Id"] = 1; // Change this value for a specific node
  doc_1["Status"] = "False";

  doc_5["Payload"] = "This is DATA for a Specific Node [5]";
  doc_5["Node_Id"] = 5; // Change this value for a specific node
  doc_5["Status"] = "False";

  doc_9["Payload"] = "This is DATA for a Specific Node [9]";
  doc_9["Node_Id"] = 9; // Change this value for a specific node
  doc_9["Status"] = "False";

  doc_13["Payload"] = "This is DATA for a Specific Node [13]";
  doc_13["Node_Id"] = 13; // Change this value for a specific node
  doc_13["Status"] = "False";

  serializeJson(doc_1, jsonString_1);
  serializeJson(doc_5, jsonString_5);
  serializeJson(doc_9, jsonString_9);
  serializeJson(doc_13, jsonString_13);

}

void loop() {

  sendData(jsonString_1, doc_1["Node_Id"]);
  delay(2000);

  sendData(jsonString_5, doc_5["Node_Id"]);
  delay(2000);

  sendData(jsonString_9, doc_9["Node_Id"]);
  delay(2000);

  sendData(jsonString_13, doc_13["Node_Id"]);
  delay(2000);

}
