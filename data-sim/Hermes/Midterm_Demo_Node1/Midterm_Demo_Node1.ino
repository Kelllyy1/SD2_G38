#include <ArduinoJson.h>
#include <Wire.h>
#include <SPIFFS.h>

#define NODE_ID 1  // This node's ID
#define ACK_TIMEOUT 500  // Timeout for acknowledgment (ms)

// Task handles for managing FreeRTOS tasks
TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;

String receivedJsonSerial = "";
String JSON_NODE_1 = "";
String JSON_NODE_4 = "";
String JSON_NODE_7 = "";
bool NODE_1_DATA_AVAILABLE = false;
bool NODE_4_DATA_AVAILABLE = false;
bool NODE_7_DATA_AVAILABLE = false;

bool ping;
String jsonData;
String Processed_JSON;
bool processing = false;

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);  // Communication with Node 5

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed!");
        return;
    }
    Serial.println("SPIFFS mounted successfully.");

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(listenForNodesTask, "ListenNodes", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(routeDataTask, "RouteData", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
}

void loop() {
    // Empty: Tasks handle execution.
}

void listenForNodesTask(void *pvParameters) {
    while (true) {
        DynamicJsonDocument doc(2048);
        receivedJsonSerial = readFromSerialStandIn();
        Serial2.println(receivedJsonSerial);
        if (receivedJsonSerial == "") {
            Serial.println("Done with all three files");
            delay(1000);
            while (true) {};
        }
        DeserializationError error = deserializeJson(doc, receivedJsonSerial);
        if (error) {
            Serial.println("Failed to parse JSON");
            Serial.println(jsonData);
        }

        int deviceID = doc["deviceID"];

        if (deviceID == 1) {
            JSON_NODE_1 = receivedJsonSerial;
            Serial.println("Node ID is 1");
            NODE_1_DATA_AVAILABLE = true;
        } else if (deviceID == 4) {
            JSON_NODE_4 = receivedJsonSerial;
            Serial.println("Node ID is 4");
            NODE_4_DATA_AVAILABLE = true;
        } else if (deviceID == 7) {
            JSON_NODE_7 = receivedJsonSerial;
            Serial.println("Node ID is 7");
            Serial.println(JSON_NODE_7);
            NODE_7_DATA_AVAILABLE = true;
        } else {
            Serial.println("Data does not belong in this row");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Prevent task starvation
    }
}

// Task to handle routing local JSON data
void routeDataTask(void *pvParameters) {
    while (true) {
        if (NODE_1_DATA_AVAILABLE) {
            Serial.println("Processing data here");
            RouteReceivedData(JSON_NODE_1);
            NODE_1_DATA_AVAILABLE = false;
        }
        if (NODE_4_DATA_AVAILABLE) {
            RouteReceivedData(JSON_NODE_4);
            NODE_4_DATA_AVAILABLE = false;
        }

        if (NODE_7_DATA_AVAILABLE) {
            RouteReceivedData(JSON_NODE_7);
            NODE_7_DATA_AVAILABLE = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // Process every second
    }
}

String readFromSerialStandIn() {
    String data = Serial.readStringUntil('\n');  // Read the data sent over serial
    Serial.println("From Node 1: Data received: " + data);

    // Prepare the JSON string
    String jsonData = data;

    return jsonData;
}

// Function to read from Serial
String readFromSerial(HardwareSerial &serial) {
    String receivedData = "";
    while (serial.available()) {
        char c = serial.read();
        receivedData += c;
    }
    receivedData.trim(); // Remove unwanted spaces or newline chars
    return receivedData;
}

// Function to route received data
bool RouteReceivedData(String jsonData) {
    String proccessed_data;
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonData);

    if (error) {
        Serial.println("Failed to parse JSON");
        Serial.println(jsonData);
        return false;
    }

    int deviceID = doc["deviceID"];

    if (deviceID == NODE_ID) {
        proccessed_data = ProcessData(doc);

        doc.clear();

        error = deserializeJson(doc, proccessed_data);

        if (error) {
            Serial.println("Failed to parse JSON after processing data");
            Serial.println(proccessed_data);
            return false;
        } else {
            Serial.println("Processed Data");
            // Save the JSON data to a file
            saveJsonToFile(proccessed_data);  // Save the processed data
            Serial.println("Sent" + proccessed_data + "to Node 4");
            Serial2.println(proccessed_data);
        }
    } else {
        Serial.println("Routed to Node 4 successfully");
        // Save the original JSON data to a file
        saveJsonToFile(jsonData);  // Save the raw data
        Serial2.println(jsonData);
        Serial.println("Sent" + jsonData + "to Node 4");
    }

    return true;  // Indicate success
}

// Function to process data
String ProcessData(DynamicJsonDocument &doc) {
    doc["processed"] = "True";
    
    DeserializationError error = serializeJson(doc, Processed_JSON);

    if (error) {
        Serial.println("Failed to serialize JSON");
    }
    return Processed_JSON;
}

// Function to save JSON data to a file
void saveJsonToFile(String jsonData) {
    // Open or create the file to save the JSON data
    File file = SPIFFS.open("/savedData.json", FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    // Write the JSON data to the file
    file.println(jsonData);
    file.close();  // Close the file
}
