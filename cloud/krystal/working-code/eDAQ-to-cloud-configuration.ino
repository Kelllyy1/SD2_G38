// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <ArduinoJson.h>
#include <Wire.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "time.h"

#define NODE_ID 5  // This node's ID
#define ACK_TIMEOUT 500  
#define LISTEN_TIMEOUT 500  
#define WAITFORDATA_TIMEOUT 500  
#define AWS_IOT_PUBLISH_TOPIC "esp/Module/bulk"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;         // Standard Time
const int daylightOffset_sec = 3600;          // Add 3600 during DST

// Global Variables
WiFiClientSecure net;
MQTTClient client(256);


SemaphoreHandle_t serial1Semaphore;
SemaphoreHandle_t serial2Semaphore;

QueueHandle_t jsonQueue;
QueueHandle_t send_node_5Queue;
QueueHandle_t processing_Queue;

// Task handles for managing FreeRTOS tasks
TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;
TaskHandle_t Task3Handle = NULL;
TaskHandle_t Task8Handle = NULL;


bool listenForPing(HardwareSerial *serialPort);
String WaitForData(HardwareSerial &serial);
String readFromSerial(HardwareSerial &serialPort);
bool waitForAck(HardwareSerial *serialPort, SemaphoreHandle_t *semaphore);
int RouteData(String json);


int Send_to_Node_9 = 0;
String Node_9_Data;


void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);  // communication with node 1
    Serial1.begin(9600, SERIAL_8N1, 4, 23);  // communication with node 9

    serial1Semaphore = xSemaphoreCreateBinary();
    serial2Semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(serial1Semaphore);
    xSemaphoreGive(serial2Semaphore);
    
    jsonQueue = xQueueCreate(10, sizeof(char *)); 
    if (jsonQueue == NULL) {
        Serial.println("jsonQueue creation failed!");
    } else {
        Serial.println("jsonQueue created successfully.");
    }

    send_node_5Queue = xQueueCreate(10, sizeof(char *)); 
    if (send_node_5Queue == NULL) {
        Serial.println("send_node_5Queue creation failed!");
    } else {
        Serial.println("send_node_5Queuecreated successfully.");
    }

    processing_Queue = xQueueCreate(10, sizeof(char *)); 
    if (processing_Queue == NULL) {
        Serial.println("processing_Queue creation failed!");
    } else {
        Serial.println("processing_Queue created successfully.");
    }

    connectWiFi();
    // TOOD: This may be a problem, because I have to the time each time the data is sent; Consider placing in the Publish function to add to the "TimeInSeconds" attribute to the json object
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // To get the epoch time in seconds
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        delay(1000);
    }

    connectAWS();
    client.onMessage(messageHandler);
    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(listenForNode1Task, "ListenNode1", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(ParseJsonTask, "Parse", 4096, NULL, 1, &Task3Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(SendToNodesTask, "SendData9", 8192, NULL, 1, &Task2Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(ProcessTask, "ProcesData", 4096, NULL, 1, &Task8Handle, 1); // Run on Core 1
}

void loop() {
    // Empty: Tasks handle execution.
}


void listenForNode1Task(void *pvParameters) {

    SemaphoreHandle_t *semaphore;

    String tempData;
    int ping;

    String serial_number;

    HardwareSerial *serialPort;

    serialPort = &Serial2;
    semaphore = &serial2Semaphore;
    serial_number = "serial2";
    while (true) {

    // Serial.println("Reached");
    Serial.println("listen to node 1 starts");

    // Check for ping message
    ping = listenForPing(serialPort);
    if (ping) {
      
      tempData = WaitForData(*serialPort);

      Serial.println("Received Data (should be JSON) = " + tempData);

        if(RouteData(tempData)) {
          Node_9_Data = tempData;
        }

        char *Data = strdup(tempData.c_str());

        if (Data == NULL) {
            Serial.println("Memory allocation failed!");
            continue; // Skip sending if allocation failed
        }

        if (xQueueSend(jsonQueue, &Data, portMAX_DELAY) == pdPASS) { // It's killing itself here
            if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
                xTaskNotifyGive(Task3Handle);
            }
        } else {
            Serial.println("Queue full! Data not sent.");
            free(Data);  // Prevent memory leak if sending fails
        }
      }
    }

      vTaskDelay(100 / portTICK_PERIOD_MS);    
  }


void SendToNodesTask(void *pvParameters) {

    SemaphoreHandle_t semaphore;

    semaphore = serial2Semaphore;

    while (true) {
        Serial.println("reached task");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing
        Serial.println("Reached after task");

        while (uxQueueMessagesWaiting(send_node_5Queue) > 0) {  // Process all messages
            char *DataSend;
            if (xQueueReceive(send_node_5Queue, &DataSend, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore

                    StaticJsonDocument<3072> message;
                    DeserializationError error = deserializeJson(message, DataSend);
                    if (!error) {
                        String msgStr = String(DataSend);
                        Serial.println("publishing message" + msgStr);

                        publishMessage(message);
                    } else {
                      // Print the error -Krystal
                      Serial.print("Error: ");
                      Serial.println(error.c_str());
                        Serial.println("Did not parse, could not send to cloud");
                        // Handle JSON parsing error (optional logging)
                    }
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
            }
        }
    }
}

void ProcessTask(void *pvParameters) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (uxQueueMessagesWaiting(processing_Queue) > 0) {
            char *Data;
            if (xQueueReceive(processing_Queue, &Data, portMAX_DELAY) == pdTRUE) {
              StaticJsonDocument<1024> doc;
              DeserializationError error = deserializeJson(doc, Data);

              if (error) {
                  Serial.print("JSON Parse Error: ");
                  Serial.println(error.c_str());
                  
              }

              JsonArray cells = doc["cells"];
              float totalVoltage = 0, totalCurrent = 0, totalTemperature = 0;
              const char* faultList[10];

              for (int i = 0; i < cells.size(); i++) {
                  totalVoltage += cells[i]["voltage"].as<float>();
                  totalCurrent += cells[i]["current"].as<float>();
                  totalTemperature += cells[i]["temperature"].as<float>();
                  faultList[i] = cells[i]["faults"].as<const char*>();
              }

              float avgVoltage = totalVoltage / cells.size();
              float avgCurrent = totalCurrent / cells.size();
              float avgTemperature = totalTemperature / cells.size();
              const char* commonFault = mostCommonFault(faultList, cells.size());

              // Create new JSON
              StaticJsonDocument<256> summaryDoc;
              summaryDoc["rack_id"] = doc["rack_id"];
              summaryDoc["module_id"] = doc["module_id"];
              summaryDoc["deviceID"] = doc["deviceID"];
              summaryDoc["avg_voltage"] = avgVoltage;
              summaryDoc["avg_current"] = avgCurrent;
              summaryDoc["avg_temperature"] = avgTemperature;
              summaryDoc["most_common_fault"] = commonFault;

              String output;
              serializeJson(summaryDoc, output);

              // Copy output into a dynamically allocated char array for queue safety
              char* jsonCopy = (char*)pvPortMalloc(output.length() + 1);
              if (jsonCopy != NULL) {
                  strcpy(jsonCopy, output.c_str());

                  if (xQueueSend(send_node_5Queue, &jsonCopy, 50 / portTICK_PERIOD_MS) == pdPASS) {
                      if (uxQueueMessagesWaiting(send_node_5Queue) == 1) {  // Notify only for first message
                          xTaskNotifyGive(Task2Handle); // Notify send_to_node_5 task
                      }
                  } else {
                      Serial.println("send_node_5Queue full! Data not sent.");
                      vPortFree(jsonCopy);  // Prevent memory leak
                  }
              } else {
                  Serial.println("Memory allocation failed!");
              }

              Serial.println("Processed JSON: \n" + output);
              free(Data);
          }
        }
    }
}

void ParseTask(void *pvParameters) {
    SemaphoreHandle_t semaphore;
    semaphore = serial2Semaphore;

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 

        if (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Check if JSON queue has messages
            char *Data;
            if (xQueueReceive(jsonQueue, &Data, portMAX_DELAY) == pdTRUE) {
                int deviceID = DeviceID(Data);
                if (deviceID == NODE_ID) {
                    // Try adding to processing queue
                    if (xQueueSend(processing_Queue, &Data, 50 / portTICK_PERIOD_MS) == pdTRUE) {
               
                        xTaskNotifyGive(Task8Handle); // process it

                    } 
                    else {
                        Serial.println("Processing queue full");
                    }
                }
                else {

                        xQueueSend(send_node_5Queue, &Data, 50 / portTICK_PERIOD_MS);
                        xTaskNotifyGive(Task2Handle); // send to node 5
                }
                // free(Data);
            }
        } 
    }
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <ArduinoJson.h>
#include <Wire.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "time.h"

#define NODE_ID 5  // This node's ID
#define ACK_TIMEOUT 500  
#define LISTEN_TIMEOUT 500  
#define WAITFORDATA_TIMEOUT 500  
#define AWS_IOT_PUBLISH_TOPIC "esp/Module/bulk"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;         // Standard Time
const int daylightOffset_sec = 3600;          // Add 3600 during DST

// Global Variables
WiFiClientSecure net;
MQTTClient client(256);


SemaphoreHandle_t serial1Semaphore;
SemaphoreHandle_t serial2Semaphore;

QueueHandle_t jsonQueue;

// Task handles for managing FreeRTOS tasks
TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;
TaskHandle_t Task3Handle = NULL;

bool listenForPing(HardwareSerial *serialPort);
String WaitForData(HardwareSerial &serial);
String readFromSerial(HardwareSerial &serialPort);
bool waitForAck(HardwareSerial *serialPort, SemaphoreHandle_t *semaphore);
int RouteData(String json);


int Send_to_Node_9 = 0;
String Node_9_Data;


void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);  // communication with node 1
    Serial1.begin(9600, SERIAL_8N1, 4, 23);  // communication with node 9

    serial1Semaphore = xSemaphoreCreateBinary();
    serial2Semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(serial1Semaphore);
    xSemaphoreGive(serial2Semaphore);
    
    jsonQueue = xQueueCreate(10, sizeof(char *)); 
    if (jsonQueue == NULL) {
        Serial.println("jsonQueue creation failed!");
    } else {
        Serial.println("jsonQueue created successfully.");
    }

    connectWiFi();
    // TOOD: This may be a problem, because I have to the time each time the data is sent; Consider placing in the Publish function to add to the "TimeInSeconds" attribute to the json object
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // To get the epoch time in seconds
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        delay(1000);
    }

    connectAWS();
    client.onMessage(messageHandler);
    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(listenForNode1Task, "ListenNode1", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(ParseJsonTask, "Parse", 4096, NULL, 1, &Task3Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(SendToNodesTask, "SendData9", 8192, NULL, 1, &Task2Handle, 1); // Run on Core 1
}

void loop() {
    // Empty: Tasks handle execution.
}


void listenForNode1Task(void *pvParameters) {

    SemaphoreHandle_t *semaphore;

    String tempData;
    int ping;

    String serial_number;

    HardwareSerial *serialPort;

    serialPort = &Serial2;
    semaphore = &serial2Semaphore;
    serial_number = "serial2";
    while (true) {

    // Serial.println("Reached");
    Serial.println("listen to node 1 starts");

    // Check for ping message
    ping = listenForPing(serialPort);
    if (ping) {
      
      tempData = WaitForData(*serialPort);

      Serial.println("Received Data (should be JSON) = " + tempData);

        if(RouteData(tempData)) {
          Node_9_Data = tempData;
        }

        char *Data = strdup(tempData.c_str());

        if (Data == NULL) {
            Serial.println("Memory allocation failed!");
            continue; // Skip sending if allocation failed
        }

        if (xQueueSend(jsonQueue, &Data, portMAX_DELAY) == pdPASS) { // It's killing itself here
            if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
                xTaskNotifyGive(Task2Handle);
            }
        } else {
            Serial.println("Queue full! Data not sent.");
            free(Data);  // Prevent memory leak if sending fails
        }
      }
    }

      vTaskDelay(100 / portTICK_PERIOD_MS);    
  }


void SendToNodesTask(void *pvParameters) {

    SemaphoreHandle_t semaphore;

    semaphore = serial2Semaphore;

    while (true) {
        Serial.println("reached task");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing
        Serial.println("Reached after task");

        while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages
            char *DataSend;
            if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore

                    StaticJsonDocument<3072> message;
                    DeserializationError error = deserializeJson(message, DataSend);
                    if (!error) {
                        String msgStr = String(DataSend);
                        Serial.println("publishing message" + msgStr);

                        publishMessage(message);
                    } else {
                      // Print the error -Krystal
                      Serial.print("Error: ");
                      Serial.println(error.c_str());
                        Serial.println("Did not parse, could not send to cloud");
                        // Handle JSON parsing error (optional logging)
                    }
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
            }
        }
    }
}

void ProcessTask(void *pvParameters) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (uxQueueMessagesWaiting(processing_Queue) > 0) {
            char *Data;
            if (xQueueReceive(processing_Queue, &Data, portMAX_DELAY) == pdTRUE) {
              StaticJsonDocument<1024> doc;
              DeserializationError error = deserializeJson(doc, Data);

              if (error) {
                  Serial.print("JSON Parse Error: ");
                  Serial.println(error.c_str());
                  
              }

              JsonArray cells = doc["cells"];
              float totalVoltage = 0, totalCurrent = 0, totalTemperature = 0;
              const char* faultList[10];

              for (int i = 0; i < cells.size(); i++) {
                  totalVoltage += cells[i]["voltage"].as<float>();
                  totalCurrent += cells[i]["current"].as<float>();
                  totalTemperature += cells[i]["temperature"].as<float>();
                  faultList[i] = cells[i]["faults"].as<const char*>();
              }

              float avgVoltage = totalVoltage / cells.size();
              float avgCurrent = totalCurrent / cells.size();
              float avgTemperature = totalTemperature / cells.size();
              const char* commonFault = mostCommonFault(faultList, cells.size());

              // Create new JSON
              StaticJsonDocument<256> summaryDoc;
              summaryDoc["rack_id"] = doc["rack_id"];
              summaryDoc["module_id"] = doc["module_id"];
              summaryDoc["deviceID"] = doc["deviceID"];
              summaryDoc["avg_voltage"] = avgVoltage;
              summaryDoc["avg_current"] = avgCurrent;
              summaryDoc["avg_temperature"] = avgTemperature;
              summaryDoc["most_common_fault"] = commonFault;

              String output;
              serializeJson(summaryDoc, output);

              // Copy output into a dynamically allocated char array for queue safety
              char* jsonCopy = (char*)pvPortMalloc(output.length() + 1);
              if (jsonCopy != NULL) {
                  strcpy(jsonCopy, output.c_str());

                  if (xQueueSend(send_node_5Queue, &jsonCopy, 50 / portTICK_PERIOD_MS) == pdPASS) {
                      if (uxQueueMessagesWaiting(send_node_5Queue) == 1) {  // Notify only for first message
                          xTaskNotifyGive(Task2Handle); // Notify send_to_node_5 task
                      }
                  } else {
                      Serial.println("send_node_5Queue full! Data not sent.");
                      vPortFree(jsonCopy);  // Prevent memory leak
                  }
              } else {
                  Serial.println("Memory allocation failed!");
              }

              Serial.println("Processed JSON: \n" + output);
              free(Data);
          }
        }
    }
}

int RouteData(String json) {
  // Added debug message to determine if JSON data is valid -Krystal
    if (!json.startsWith("{") || !json.endsWith("}")) {
        Serial.println("Invalid JSON format!");
        // return -1;  // return early or handle error
    }

  int DeviceID;
  StaticJsonDocument<200> doc;  // Adjust size as needed

  DeserializationError error = deserializeJson(doc, json);
  if (error) {
      Serial.println("JSON parsing failed!");
    // Added debug message to print error -Krystal
      Serial.print("Error: ");
      Serial.println(error.c_str());
  }

  Serial.println("Raw JSON: " + json);

  DeviceID = doc["deviceID"];
  if (DeviceID % 4 ==1){
      Send_to_Node_9 = 1;
      return 1; 
  }
  else {
    return 0;
  }
}


String WaitForData(HardwareSerial &serial) {

    String serial_number;
    String receivedJsonSerial;

    // Alternate between Serial1 and Serial2
    if (&serial == &Serial2) {
        serial_number = "serial2";
    } else {
        serial_number = "serial1";
    }

    String listen;
    listen = "";
    unsigned long startTime = millis();  // Record the start time
    while (true) {
    serial.println("Available");
    listen = readFromSerial(serial); 
    if (listen != "PING" && listen != "" &&  listen.startsWith("{") && listen.endsWith("}")) {
        Serial.println("Did not receive PING, instead received: " + listen + "from " + serial_number);
        receivedJsonSerial = listen;
        return receivedJsonSerial;
    }

    // Print status every time a PING is still being received
    Serial.println("receiving" + listen + " PING after Available was sent to " + serial_number);

    // Check if the 500 ms timeout has passed
    if (millis() - startTime >= WAITFORDATA_TIMEOUT) {
        Serial.println("Timeout reached, stopping PING loop.");
        return "no data";
    }
  
  }
    
}


bool listenForPing(HardwareSerial *serialPort) {
    String listen = "";
    unsigned long startTime = millis();  

    String serial_number;

    if (serialPort == &Serial1) {
        serial_number = "serial1";
    } else if (serialPort == &Serial2) {
        serial_number = "serial2";
    } 

    while (millis() - startTime < LISTEN_TIMEOUT) {  
        listen = readFromSerial(*serialPort);  
        listen.trim();  
        
        Serial.println("Listening for PING from " + serial_number + ": [" + listen + "]");  

        if (listen == "PING") {
            Serial.println("Received PING");
            serialPort->println("Available");  
 
            return true;  
        }
        vTaskDelay(1 / portTICK_PERIOD_MS); 
    }

    Serial.println("Did not receive PING within timeout from " + serial_number);
    return false;
}


String readFromSerial(HardwareSerial &serial) {

    String receivedData = "";

    // Take the semaphore before accessing the serial line 
    while (serial.available()) {
        char c = serial.read();

        if (c == '\n' || c == '\r') {
            break;  // Stop reading at newline or carriage return
        }

        receivedData += c;
        delay(2);  // Use delay to prevent task starvation
    }
    receivedData.trim();  // Remove unwanted spaces or newline chars
    return receivedData;
}


bool waitForAck(HardwareSerial *serialPort, SemaphoreHandle_t *semaphore) {

    String serial_number;

    if (serialPort == &Serial1) {
        serial_number = "serial1";
    } else if (serialPort == &Serial2) {
        serial_number = "serial2";
    } 
    

    unsigned long startTime = millis();
    Serial.println("Sent: PING to " + serial_number);
    while (millis() - startTime < ACK_TIMEOUT) {
        serialPort->println("PING");
        delay(50);
        String receivedData = readFromSerial(*serialPort);
        if (receivedData == "Available") {
            Serial.println(serial_number + "responded with available: ");
            return true;
        }
        else if (receivedData == "PING") {
          xSemaphoreGive(*semaphore);
          vTaskDelay(2000 / portTICK_PERIOD_MS);
          xSemaphoreTake(*semaphore, portMAX_DELAY);
          
        }
        else {
          Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
        }
    }
    Serial.println(serial_number + "did not respond in time.");
    return false;
}



String generateRandomID() {
    char idBuffer[20];
    snprintf(idBuffer, sizeof(idBuffer), "B%03d-R%03d-M%03d-C%03d",
             random(1, 1000), random(1, 1000), random(1, 1000), random(1, 1000));
    return String(idBuffer);
}

// Function to connect ESP32 to Wi-Fi -Krystal
void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        // delay();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Serial.println("\nWi-Fi Connected!");
}


// Function to improve my way of connecting to AWS; so I don't have to reset after each connection
void connectAWS() {
    // Load AWS Certificates
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // Configure MQTT Client
    client.begin(AWS_IOT_ENDPOINT, 8883, net);

    Serial.print("Connecting to AWS IoT...");
    unsigned long startAttemptTime = millis();
    const unsigned long connectionTimeout = 10000; // 10 seconds timeout

    while (!client.connect(THINGNAME)) {
        Serial.print(".");
        delay(100);  // Retry every second

        if (millis() - startAttemptTime > connectionTimeout) {
            Serial.println("\nAWS IoT Connection Timeout!");
            Serial.print("MQTT Error Code: ");
            Serial.println(client.lastError());
            return;
        }
    }

    Serial.println("\n Connected to AWS IoT!");

    // Subscribe to the topic after successful connection
    Serial.print("Subscribing to topic: ");
    Serial.println(AWS_IOT_PUBLISH_TOPIC);
    client.subscribe(AWS_IOT_PUBLISH_TOPIC);
}


// Function to publish data to AWS IoT
void publishMessage(const JsonDocument& doc) {
    if (!client.connected()) {
        Serial.println("Reconnecting to AWS IoT...");
        if (!client.connect(THINGNAME)) {
            Serial.println("Reconnection failed!");
            return;
        }
    }

    char jsonOutput[2048];
    serializeJson(doc, jsonOutput);

    Serial.println("Publishing message: ");
    serializeJsonPretty(doc, Serial);
    Serial.println();

    if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonOutput)) {
        Serial.println("Publish successful!");
    } else {
        Serial.print("Publish failed! MQTT Error Code: ");
        Serial.println(client.lastError());
    }
}


// Function to handle incoming messages from AWS IoT
void messageHandler(String &topic, String &payload) {
    Serial.println("Incoming message:");
    Serial.println("Topic: " + topic);
    Serial.println("Payload: " + payload);
}

int RouteData(String json) {
  // Added debug message to determine if JSON data is valid -Krystal
    if (!json.startsWith("{") || !json.endsWith("}")) {
        Serial.println("Invalid JSON format!");
        // return -1;  // return early or handle error
    }

  int DeviceID;
  StaticJsonDocument<200> doc;  // Adjust size as needed

  DeserializationError error = deserializeJson(doc, json);
  if (error) {
      Serial.println("JSON parsing failed!");
    // Added debug message to print error -Krystal
      Serial.print("Error: ");
      Serial.println(error.c_str());
  }

  Serial.println("Raw JSON: " + json);

  DeviceID = doc["deviceID"];
  if (DeviceID % 4 ==1){
      Send_to_Node_9 = 1;
      return 1; 
  }
  else {
    return 0;
  }
}


String WaitForData(HardwareSerial &serial) {

    String serial_number;
    String receivedJsonSerial;

    // Alternate between Serial1 and Serial2
    if (&serial == &Serial2) {
        serial_number = "serial2";
    } else {
        serial_number = "serial1";
    }

    String listen;
    listen = "";
    unsigned long startTime = millis();  // Record the start time
    while (true) {
    serial.println("Available");
    listen = readFromSerial(serial); 
    if (listen != "PING" && listen != "" &&  listen.startsWith("{") && listen.endsWith("}")) {
        Serial.println("Did not receive PING, instead received: " + listen + "from " + serial_number);
        receivedJsonSerial = listen;
        return receivedJsonSerial;
    }

    // Print status every time a PING is still being received
    Serial.println("receiving" + listen + " PING after Available was sent to " + serial_number);

    // Check if the 500 ms timeout has passed
    if (millis() - startTime >= WAITFORDATA_TIMEOUT) {
        Serial.println("Timeout reached, stopping PING loop.");
        return "no data";
    }
  
  }
    
}


bool listenForPing(HardwareSerial *serialPort) {
    String listen = "";
    unsigned long startTime = millis();  

    String serial_number;

    if (serialPort == &Serial1) {
        serial_number = "serial1";
    } else if (serialPort == &Serial2) {
        serial_number = "serial2";
    } 

    while (millis() - startTime < LISTEN_TIMEOUT) {  
        listen = readFromSerial(*serialPort);  
        listen.trim();  
        
        Serial.println("Listening for PING from " + serial_number + ": [" + listen + "]");  

        if (listen == "PING") {
            Serial.println("Received PING");
            serialPort->println("Available");  
 
            return true;  
        }
        vTaskDelay(1 / portTICK_PERIOD_MS); 
    }

    Serial.println("Did not receive PING within timeout from " + serial_number);
    return false;
}


String readFromSerial(HardwareSerial &serial) {

    String receivedData = "";

    // Take the semaphore before accessing the serial line 
    while (serial.available()) {
        char c = serial.read();

        if (c == '\n' || c == '\r') {
            break;  // Stop reading at newline or carriage return
        }

        receivedData += c;
        delay(2);  // Use delay to prevent task starvation
    }
    receivedData.trim();  // Remove unwanted spaces or newline chars
    return receivedData;
}


bool waitForAck(HardwareSerial *serialPort, SemaphoreHandle_t *semaphore) {

    String serial_number;

    if (serialPort == &Serial1) {
        serial_number = "serial1";
    } else if (serialPort == &Serial2) {
        serial_number = "serial2";
    } 
    

    unsigned long startTime = millis();
    Serial.println("Sent: PING to " + serial_number);
    while (millis() - startTime < ACK_TIMEOUT) {
        serialPort->println("PING");
        delay(50);
        String receivedData = readFromSerial(*serialPort);
        if (receivedData == "Available") {
            Serial.println(serial_number + "responded with available: ");
            return true;
        }
        else if (receivedData == "PING") {
          xSemaphoreGive(*semaphore);
          vTaskDelay(2000 / portTICK_PERIOD_MS);
          xSemaphoreTake(*semaphore, portMAX_DELAY);
          
        }
        else {
          Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
        }
    }
    Serial.println(serial_number + "did not respond in time.");
    return false;
}



String generateRandomID() {
    char idBuffer[20];
    snprintf(idBuffer, sizeof(idBuffer), "B%03d-R%03d-M%03d-C%03d",
             random(1, 1000), random(1, 1000), random(1, 1000), random(1, 1000));
    return String(idBuffer);
}

// Function to connect ESP32 to Wi-Fi -Krystal
void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        // delay();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Serial.println("\nWi-Fi Connected!");
}


// Function to improve my way of connecting to AWS; so I don't have to reset after each connection
void connectAWS() {
    // Load AWS Certificates
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // Configure MQTT Client
    client.begin(AWS_IOT_ENDPOINT, 8883, net);

    Serial.print("Connecting to AWS IoT...");
    unsigned long startAttemptTime = millis();
    const unsigned long connectionTimeout = 10000; // 10 seconds timeout

    while (!client.connect(THINGNAME)) {
        Serial.print(".");
        delay(100);  // Retry every second

        if (millis() - startAttemptTime > connectionTimeout) {
            Serial.println("\nAWS IoT Connection Timeout!");
            Serial.print("MQTT Error Code: ");
            Serial.println(client.lastError());
            return;
        }
    }

    Serial.println("\n Connected to AWS IoT!");

    // Subscribe to the topic after successful connection
    Serial.print("Subscribing to topic: ");
    Serial.println(AWS_IOT_PUBLISH_TOPIC);
    client.subscribe(AWS_IOT_PUBLISH_TOPIC);
}


// Function to publish data to AWS IoT
void publishMessage(const JsonDocument& doc) {
    if (!client.connected()) {
        Serial.println("Reconnecting to AWS IoT...");
        if (!client.connect(THINGNAME)) {
            Serial.println("Reconnection failed!");
            return;
        }
    }

    char jsonOutput[2048];
    serializeJson(doc, jsonOutput);

    Serial.println("Publishing message: ");
    serializeJsonPretty(doc, Serial);
    Serial.println();

    if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonOutput)) {
        Serial.println("Publish successful!");
    } else {
        Serial.print("Publish failed! MQTT Error Code: ");
        Serial.println(client.lastError());
    }
}


// Function to handle incoming messages from AWS IoT
void messageHandler(String &topic, String &payload) {
    Serial.println("Incoming message:");
    Serial.println("Topic: " + topic);
    Serial.println("Payload: " + payload);
}
