// Iteration 2
#include <ArduinoJson.h>
#include <Wire.h>
// #include "SC16IS752.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <map>

#define NODE_ID 1  // This node's ID
#define ACK_TIMEOUT 500  // Timeout for acknowledgment (ms)
#define LISTEN_TIMEOUT 500
#define WAITFORDATA_TIMEOUT 500  


SemaphoreHandle_t serial1Semaphore;
SemaphoreHandle_t serial2Semaphore;
QueueHandle_t jsonQueue;
QueueHandle_t retry_node_5Queue;
QueueHandle_t retry_node_2Queue;
QueueHandle_t send_node_5Queue;
QueueHandle_t send_node_2Queue;
QueueHandle_t processing_Queue;

// Task handles for managing FreeRTOS tasks
TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;
TaskHandle_t Task3Handle = NULL;
TaskHandle_t Task4Handle = NULL;
TaskHandle_t Task5Handle = NULL;
TaskHandle_t Task6Handle = NULL;
TaskHandle_t Task7Handle = NULL;
TaskHandle_t Task8Handle = NULL;


bool listenForPing(HardwareSerial *serialPort);
String WaitForData(HardwareSerial &serial);
String readFromSerial(HardwareSerial &serialPort);
bool waitForAck(HardwareSerial *serialPort);
String CreateJson();
String createBatteryJson1();
String createBatteryJson2();
String createBatteryJson3();
int DeviceID(String json);
const char* mostCommonFault(const char* faults[], int size);



void setup() {

    Serial.begin(115200);
    Serial.println("Start of experiement \n\n\n\n\n\n");
    Serial2.begin(9600, SERIAL_8N1, 16, 17);  
    Serial1.begin(9600, SERIAL_8N1, 4, 23);  

    jsonQueue = xQueueCreate(10, sizeof(char *)); 
    if (jsonQueue == NULL) {
        Serial.println("Queue creation failed!");
    } else {
        Serial.println("Queue created successfully.");
    }

    retry_node_5Queue = xQueueCreate(10, sizeof(char *)); 
    if (retry_node_5Queue == NULL) {
        Serial.println("retry_node_5 Queuecreation failed!");
    } else {
        Serial.println("retry_node_5Queue created successfully.");
    }


    retry_node_2Queue = xQueueCreate(10, sizeof(char *)); 
    if (retry_node_2Queue == NULL) {
        Serial.println("retry_node_2Queue creation failed!");
    } else {
        Serial.println("retry_node_2Queue created successfully.");
    }

    send_node_5Queue = xQueueCreate(10, sizeof(char *)); 
    if (send_node_5Queue == NULL) {
        Serial.println("send_node_5Queue creation failed!");
    } else {
        Serial.println("send_node_5Queuecreated successfully.");
    }


    send_node_2Queue = xQueueCreate(10, sizeof(char *)); 
    if (send_node_2Queue == NULL) {
        Serial.println("send_node_2Queue creation failed!");
    } else {
        Serial.println("send_node_2Queuecreated successfully.");
    }

    processing_Queue = xQueueCreate(10, sizeof(char *)); 
    if (processing_Queue == NULL) {
        Serial.println("processing_Queue creation failed!");
    } else {
        Serial.println("processing_Queue created successfully.");
    }


    serial1Semaphore = xSemaphoreCreateBinary();
    serial2Semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(serial1Semaphore);
    xSemaphoreGive(serial2Semaphore);

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(listenForNodesTask, "listenForNode5", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0t
    // xTaskCreatePinnedToCore(listenForNode2Task, "listenForNode2", 4096, NULL, 1, &Task4Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(ParseTask, "Parse", 4096, NULL, 1, &Task5Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(SendToNode5Task, "SendData", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(RetrySend5Task, "RetrySend", 4096, NULL, 1, &Task3Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(SendToNode2Task, "SendData", 4096, NULL, 1, &Task6Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(RetrySend2Task, "RetrySend", 4096, NULL, 1, &Task7Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(ProcessTask, "ProcesData", 4096, NULL, 1, &Task8Handle, 1); // Run on Core 1
}

void loop() {
    // Empty: Tasks handle execution.
}

int i = 0;

void listenForNodesTask(void *pvParameters) {
    while (true) {
        // Serial.println("listenfornodestask");
        String tempData;

        if (i == 0) {
            tempData = createBatteryJson1();
            i++;
        } else if (i == 1) {
            tempData = createBatteryJson2();
            i++;
        } else if (i == 2) {
            tempData = createBatteryJson3();
            i=0;
        }

        // Allocate memory dynamically
        char *Data = strdup(tempData.c_str());

        if (Data == NULL) {
            Serial.println("Memory allocation failed!");
            continue; // Skip sending if allocation failed
        }

        if (xQueueSend(jsonQueue, &Data, 50 / portTICK_PERIOD_MS) == pdPASS) {
            if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
                xTaskNotifyGive(Task5Handle);
            }
        } else {
            Serial.println("jsonQueue! Data not sent.");
            free(Data);  // Prevent memory leak if sending fails
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void listenForNode2Task(void *pvParameters) {

  
    SemaphoreHandle_t *semaphore;

    String tempData;
    int ping;

    String serial_number;

    HardwareSerial *serialPort;

    serialPort = &Serial1;
    semaphore = &serial1Semaphore;
    serial_number = "serial1";
    while (true) {

    
    Serial.println("listen to node 1 starts");



    // Check for ping message
  
    ping = listenForPing(serialPort);
    if (ping) {
      
      tempData = WaitForData(*serialPort);

      Serial.println("Received Data (should be JSON) = " + tempData);


        char *Data = strdup(tempData.c_str());

        if (Data == NULL) {
            Serial.println("Memory allocation failed!");
            continue; // Skip sending if allocation failed
        }

        if (xQueueSend(send_node_5Queue, &Data, 50 / portTICK_PERIOD_MS) == pdPASS) {
            if (uxQueueMessagesWaiting(send_node_5Queue) == 1) {  // Notify only for first message
                xTaskNotifyGive(Task2Handle); // send to node 5
            }
        } else {
            Serial.println("send_node_5Queue full! Data not sent.");
            free(Data);  
        }
      }




    }


      vTaskDelay(100 / portTICK_PERIOD_MS);
        
  }


void SendToNode5Task(void *pvParameters) {

    SemaphoreHandle_t semaphore;


    semaphore = serial2Semaphore;

    HardwareSerial &serial = Serial2;

    String serial_name = "Serial2";
    

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing

        while (uxQueueMessagesWaiting(send_node_5Queue) > 0) {  // Process all messages
            char *DataSend;
            if (xQueueReceive(send_node_5Queue, &DataSend, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
                    bool ack = waitForAck(&serial);

                    if (ack) {
                        Serial2.println(DataSend);
                        Serial.println("Sent JSON to " + serial_name+ ": \n" + String(DataSend));
                        free(DataSend);
                    } else {
                        // Serial.println("Ack not received! Retrying...");
                        Serial.println("Did not send: " + String(DataSend));

                        if (xQueueSend(retry_node_5Queue, &DataSend, 50 / portTICK_PERIOD_MS) != pdTRUE) {
                            Serial.println("Failed to send to retry queue. Message lost meant for " + serial_name);
                            free(DataSend);
                        } else {
                            Serial.println("Message moved to retry queue after attempting to send to " + serial_name);
                        }
                    }
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
            }
        }
    }
}


void SendToNode2Task(void *pvParameters) {

    SemaphoreHandle_t semaphore;


    semaphore = serial1Semaphore;

    HardwareSerial &serial = Serial1;

    String serial_name = "Serial1";
    

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing

        while (uxQueueMessagesWaiting(send_node_2Queue) > 0) {  // Process all messages
            char *DataSend;
            if (xQueueReceive(send_node_2Queue, &DataSend, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
                    bool ack = waitForAck(&serial);

                    if (ack) {
                        Serial2.println(DataSend);
                        Serial.println("Sent JSON to " + serial_name+ ": \n" + String(DataSend));
                        free(DataSend);
                    } else {
                        // Serial.println("Ack not received! Retrying...");
                        Serial.println("Did not send: " + String(DataSend));

                        if (xQueueSend(retry_node_2Queue, &DataSend, 50 / portTICK_PERIOD_MS) != pdTRUE) {
                            Serial.println("Failed to send to retry queue. Message lost meant for " + serial_name);
                            free(DataSend);
                        } else {
                            Serial.println("Message moved to retry queue after attempting to send to " + serial_name);
                        }
                    }
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
            }
        }
    }
}

void RetrySend5Task(void *pvParameters) {

  
    SemaphoreHandle_t semaphore;


    semaphore = serial2Semaphore;
    
    HardwareSerial &serial = Serial2;
    String serial_name = "Serial2";

    while (true) {
        if (uxQueueMessagesWaiting(retry_node_5Queue) > 0) {  // Check if retry queue has messages
            char *retryData;
            if (xQueueReceive(retry_node_5Queue, &retryData, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
                    bool ack = waitForAck(&serial);

                    if (ack) {
                        Serial2.println(retryData);
                        Serial.println("Resent JSON to " + serial_name+ "\n" + String(retryData));
                        free(retryData);
                    } else {
                        Serial.println("Resend failed to " + serial_name+ ", keeping in retry queue.");
                        xQueueSend(retry_node_2Queue, &retryData, 50 / portTICK_PERIOD_MS);
                    }
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
            }
        } 
    }
}


void RetrySend2Task(void *pvParameters) {

  
    SemaphoreHandle_t semaphore;


    semaphore = serial1Semaphore;
    
    HardwareSerial &serial = Serial1;
    String serial_name = "Serial1";

    while (true) {
        if (uxQueueMessagesWaiting(retry_node_2Queue) > 0) {  // Check if retry queue has messages
            char *retryData;
            if (xQueueReceive(retry_node_2Queue, &retryData, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
                    bool ack = waitForAck(&serial);

                    if (ack) {
                        Serial2.println(retryData);
                        Serial.println("Resent JSON to " + serial_name+ "\n" + String(retryData));
                        free(retryData);
                    } else {
                        Serial.println("Resend failed to " + serial_name+ ", keeping in retry queue.");
                        xQueueSend(retry_node_2Queue, &retryData, 50 / portTICK_PERIOD_MS);
                    }
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
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


int DeviceID(String json) {
    StaticJsonDocument<200> doc; 
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
        // return -1; // Return an error code
    }

    if (doc.containsKey("deviceID")) {
        return doc["deviceID"].as<int>();
    } else {
        Serial.println("deviceID not found");
        return -1; // Return an error code if not found
    }
}

String ReadFromSimulator() {
    static String buffer = "";  // Static buffer to hold partial data between function calls
    String jsonData = "";

    // Keep reading the buffer until it's empty
    while (Serial.available() > 0) {
        char incomingChar = (char)Serial.read();  // Read one byte at a time
        buffer += incomingChar;  // Append it to the buffer

        // Check if we've received a complete line (i.e., contains a newline character)
        if (incomingChar == '\n') {
            jsonData = buffer;  // Set jsonData to the full message
            buffer = "";  // Reset the buffer for the next message
        }
    }

    // Return the complete message (if any) or an empty string
    return jsonData;
}


String WaitForData(HardwareSerial &serial) {

    String serial_number;   
    String listen;
    String receivedJsonSerial;
    listen = "";

    // Alternate between Serial1 and Serial2
    if (&serial == &Serial2) {
        serial_number = "serial2";
    } else {
        serial_number = "serial1";
    }


    unsigned long startTime = millis();  // Record the start time
    while (true) {
    listen = readFromSerial(serial); 
    if (listen != "PING" && listen != "") {
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


const char* mostCommonFault(const char* faults[], int size) {
    std::map<String, int> faultCount;
    int maxCount = 0, secondMaxCount = 0;
    const char* mostCommon = "Normal";
    const char* secondMostCommon = "Normal";

    for (int i = 0; i < size; i++) {
        faultCount[faults[i]]++;

        if (faultCount[faults[i]] > maxCount) {
            if (String(faults[i]) != "Normal") {
                secondMostCommon = mostCommon;
                secondMaxCount = maxCount;
                mostCommon = faults[i];
                maxCount = faultCount[faults[i]];
            }
        } else if (faultCount[faults[i]] > secondMaxCount && String(faults[i]) != "Normal") {
            secondMostCommon = faults[i];
            secondMaxCount = faultCount[faults[i]];
        }
    }

    return (String(mostCommon) == "Normal") ? secondMostCommon : mostCommon;
}


String createBatteryJson1() {
    StaticJsonDocument<1024> doc;

    doc["rack_id"] = "R001";
    doc["module_id"] = "M001";
    doc["deviceID"] = 1;

    JsonArray cells = doc.createNestedArray("cells");
    const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
                          "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
    float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
    float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
    float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
    const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
    const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

    for (int i = 0; i < 10; i++) {
        JsonObject cell = cells.createNestedObject();
        cell["id"] = ids[i];
        cell["voltage"] = voltages[i];
        cell["current"] = currents[i];
        cell["temperature"] = temperatures[i];
        cell["status"] = statuses[i];
        cell["faults"] = faults[i];
    }

    String output;
    serializeJson(doc, output);
    return output;
}


String createBatteryJson2() {
    StaticJsonDocument<1024> doc;

    doc["rack_id"] = "R002";
    doc["module_id"] = "M001";
    doc["deviceID"] = 9;

    JsonArray cells = doc.createNestedArray("cells");
    const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
                          "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
    float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
    float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
    float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
    const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
    const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

    for (int i = 0; i < 10; i++) {
        JsonObject cell = cells.createNestedObject();
        cell["id"] = ids[i];
        cell["voltage"] = voltages[i];
        cell["current"] = currents[i];
        cell["temperature"] = temperatures[i];
        cell["status"] = statuses[i];
        cell["faults"] = faults[i];
    }

    String output;
    serializeJson(doc, output);
    return output;
}



String createBatteryJson3() {
    StaticJsonDocument<1024> doc;

    doc["rack_id"] = "R003";
    doc["module_id"] = "M001";
    doc["deviceID"] = 5;

    JsonArray cells = doc.createNestedArray("cells");
    const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
                          "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
    float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
    float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
    float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
    const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
    const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

    for (int i = 0; i < 10; i++) {
        JsonObject cell = cells.createNestedObject();
        cell["id"] = ids[i];
        cell["voltage"] = voltages[i];
        cell["current"] = currents[i];
        cell["temperature"] = temperatures[i];
        cell["status"] = statuses[i];
        cell["faults"] = faults[i];
    }

    String output;
    serializeJson(doc, output);
    return output;
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



bool waitForAck(HardwareSerial *serialPort) {

    String serial_number;
    String receivedData;

    if (serialPort == &Serial1) {
        serial_number = "serial1";
    } else if (serialPort == &Serial2) {
        serial_number = "serial2";
    } 
    

    unsigned long startTime = millis();
    Serial.println("Sent: PING to " + serial_number);
    while (millis() - startTime < ACK_TIMEOUT) {
        serialPort->println("PING");
        vTaskDelay(20 / portTICK_PERIOD_MS); 
        String receivedData = readFromSerial(*serialPort);
        if (receivedData == "Available") {
            Serial.println("responded with available: ");
            return true;
        }
        else {
          Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
        }
    }
    Serial.println("did not respond in time.");
    return false;
}




// Iteration 1
// #include <ArduinoJson.h>
// #include <Wire.h>
// // #include "SC16IS752.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"

// #define NODE_ID 1  // This node's ID
// #define ACK_TIMEOUT 500  // Timeout for acknowledgment (ms)
// #define LISTEN_TIMEOUT 500  
// #define WAITFORDATA_TIMEOUT 500  


// SemaphoreHandle_t serial1Semaphore;
// SemaphoreHandle_t serial2Semaphore;
// QueueHandle_t jsonQueue;
// QueueHandle_t retryQueue;

// // Task handles for managing FreeRTOS tasks
// TaskHandle_t Task1Handle = NULL;
// TaskHandle_t Task2Handle = NULL;
// TaskHandle_t Task3Handle = NULL;

// bool listenForPing(HardwareSerial *serialPort);
// String WaitForData(HardwareSerial &serial);
// String readFromSerial(HardwareSerial &serialPort);
// bool waitForAck(HardwareSerial *serialPort);
// String CreateJson();
// String createBatteryJson1();
// String createBatteryJson2();
// String createBatteryJson3();


// String Node_4_Data;
// bool Send_to_Node_4;


// void setup() {
//     Serial.begin(115200);
//     Serial2.begin(9600, SERIAL_8N1, 16, 17);  
//     Serial1.begin(9600, SERIAL_8N1, 4, 23);  

//     jsonQueue = xQueueCreate(10, sizeof(char *)); 
//     if (jsonQueue == NULL) {
//         Serial.println("Queue creation failed!");
//     } else {
//         Serial.println("Queue created successfully.");
//     }

//     retryQueue = xQueueCreate(10, sizeof(char *)); 
//     if (retryQueue == NULL) {
//         Serial.println("Retry Queue creation failed!");
//     } else {
//         Serial.println("Retry Queue created successfully.");
//     }


//     serial1Semaphore = xSemaphoreCreateBinary();
//     serial2Semaphore = xSemaphoreCreateBinary();
//     xSemaphoreGive(serial1Semaphore);
//     xSemaphoreGive(serial2Semaphore);

//     // Create FreeRTOS tasks
//     xTaskCreatePinnedToCore(listenForNodesTask, "listenForNodes", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0
//     xTaskCreatePinnedToCore(SendToNodesTask, "SendData", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
//     xTaskCreatePinnedToCore(RetrySendTask, "RetrySend", 4096, NULL, 1, &Task3Handle, 1); // Run on Core 1
// }

// void loop() {
//     // Empty: Tasks handle execution.
// }

// int i = 0;

// void listenForNodesTask(void *pvParameters) {
//     while (true) {
//         String tempData;

//         if (i == 0) {
//             tempData = createBatteryJson1();
//             i++;
//         } else if (i == 1) {
//             tempData = createBatteryJson2();
//             i++;
//         } else if (i == 2) {
//             tempData = createBatteryJson3();
//             i=0;
//         }

//         // Allocate memory dynamically
//         char *Data = strdup(tempData.c_str());

//         if (Data == NULL) {
//             Serial.println("Memory allocation failed!");
//             continue; // Skip sending if allocation failed
//         }

//         if (xQueueSend(jsonQueue, &Data, portMAX_DELAY) == pdPASS) {
//             if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
//                 xTaskNotifyGive(Task2Handle);
//             }
//         } else {
//             Serial.println("Queue full! Data not sent.");
//             free(Data);  // Prevent memory leak if sending fails
//         }

//         vTaskDelay(500 / portTICK_PERIOD_MS);
//     }
// }


// void SendToNodesTask(void *pvParameters) {

//     SemaphoreHandle_t semaphore;


//     semaphore = serial2Semaphore;

//     while (true) {
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing

//         while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages
//             char *DataSend;
//             if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
//                 if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
//                     bool ack = waitForAck(&Serial2);

//                     if (ack) {
//                         Serial2.println(DataSend);
//                         Serial.println("Sent JSON: \n" + String(DataSend));
//                         free(DataSend);
//                     } else {
//                         // Serial.println("Ack not received! Retrying...");
//                         Serial.println("Did not send: " + String(DataSend));

//                         if (xQueueSend(retryQueue, &DataSend, 10 / portTICK_PERIOD_MS) != pdTRUE) {
//                             Serial.println("Failed to send to retry queue. Message lost.");
//                             free(DataSend);
//                         } else {
//                             Serial.println("Message moved to retry queue.");
//                         }
//                     }
//                     xSemaphoreGive(semaphore);  // Release semaphore
//                     vTaskDelay(100 / portTICK_PERIOD_MS); 
//                 } 
//             }
//         }
//     }
// }

// void RetrySendTask(void *pvParameters) {

  
//     SemaphoreHandle_t semaphore;


//     semaphore = serial2Semaphore;

//     while (true) {
//         if (uxQueueMessagesWaiting(retryQueue) > 0) {  // Check if retry queue has messages
//             char *retryData;
//             if (xQueueReceive(retryQueue, &retryData, portMAX_DELAY) == pdTRUE) {
//                 if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
//                     bool ack = waitForAck(&Serial2);

//                     if (ack) {
//                         Serial2.println(retryData);
//                         Serial.println("Resent JSON: \n" + String(retryData));
//                         free(retryData);
//                     } else {
//                         Serial.println("Resend failed, keeping in retry queue.");
//                         xQueueSend(retryQueue, &retryData, portMAX_DELAY);
//                     }
//                     xSemaphoreGive(semaphore);  // Release semaphore
//                     vTaskDelay(100 / portTICK_PERIOD_MS); 
//                 } 
//             }
//         } 
//     }
// }


// String ReadFromSimulator() {
//     static String buffer = "";  // Static buffer to hold partial data between function calls
//     String jsonData = "";

//     // Keep reading the buffer until it's empty
//     while (Serial.available() > 0) {
//         char incomingChar = (char)Serial.read();  // Read one byte at a time
//         buffer += incomingChar;  // Append it to the buffer

//         // Check if we've received a complete line (i.e., contains a newline character)
//         if (incomingChar == '\n') {
//             jsonData = buffer;  // Set jsonData to the full message
//             buffer = "";  // Reset the buffer for the next message
//         }
//     }

//     // Return the complete message (if any) or an empty string
//     return jsonData;
// }


// String WaitForData(HardwareSerial &serial) {

//     String serial_number;   
//     String listen;
//     String receivedJsonSerial;
//     listen = "";

//     // Alternate between Serial1 and Serial2
//     if (&serial == &Serial2) {
//         serial_number = "serial2";
//     } else {
//         serial_number = "serial1";
//     }


//     unsigned long startTime = millis();  // Record the start time
//     while (true) {
//     listen = readFromSerial(serial); 
//     if (listen != "PING" && listen != "") {
//         Serial.println("Did not receive PING, instead received: " + listen + "from " + serial_number);
//         receivedJsonSerial = listen;
//         return receivedJsonSerial;
//     }

//     // Print status every time a PING is still being received
//     Serial.println("receiving" + listen + " PING after Available was sent to " + serial_number);

//     // Check if the 500 ms timeout has passed
//     if (millis() - startTime >= WAITFORDATA_TIMEOUT) {
//         Serial.println("Timeout reached, stopping PING loop.");
//         return "no data";
//     }

      
//   }
// }


    


// bool listenForPing(HardwareSerial *serialPort) {
//     String listen = "";
//     unsigned long startTime = millis();  
//     String serial_number;

//     if (serialPort == &Serial1) {
//         serial_number = "serial1";
//     } else if (serialPort == &Serial2) {
//         serial_number = "serial2";
//     } 

    
    
//     while (millis() - startTime < LISTEN_TIMEOUT) {  
//         listen = readFromSerial(*serialPort);  
//         listen.trim();  
        
//         Serial.println("Listening for PING from " + serial_number + ": [" + listen + "]");  

//         if (listen == "PING") {
//             Serial.println("Received PING");
//             serialPort->println("Available");  

            
//             return true;  
//         }

//         vTaskDelay(1 / portTICK_PERIOD_MS); 
//     }

//     Serial.println("Did not receive PING within timeout from " + serial_number);
        
    

//     return false;
// }




// String createBatteryJson1() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R001";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }


// String createBatteryJson2() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R002";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }



// String createBatteryJson3() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R003";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }




// String readFromSerial(HardwareSerial &serial) {


//     String receivedData = "";

//     // Take the semaphore before accessing the serial line
    
//     while (serial.available()) {
//         char c = serial.read();

//         if (c == '\n' || c == '\r') {
//             break;  // Stop reading at newline or carriage return
//         }

//         receivedData += c;
//         delay(2);  // Use delay to prevent task starvation
//     }
//     receivedData.trim();  // Remove unwanted spaces or newline chars


    

//     return receivedData;
// }


// String CreateJson() {
//     // Generate local JSON data
//     String jsonData;
//     StaticJsonDocument<200> localJson;
//     localJson["deviceID"] = 4;  
//     localJson["temperature"] = 25.4;  
//     localJson["voltage"] = 3.7;  

//     serializeJson(localJson, jsonData);
//     return jsonData;

// }

// bool waitForAck(HardwareSerial *serialPort) {

//     String serial_number;
//     String receivedData;

//     if (serialPort == &Serial1) {
//         serial_number = "serial1";
//     } else if (serialPort == &Serial2) {
//         serial_number = "serial2";
//     } 
    

//     unsigned long startTime = millis();
//     Serial.println("Sent: PING to " + serial_number);
//     while (millis() - startTime < ACK_TIMEOUT) {
//         serialPort->println("PING");
//         vTaskDelay(20 / portTICK_PERIOD_MS); 
//         String receivedData = readFromSerial(*serialPort);
//         if (receivedData == "Available") {
//             Serial.println("responded with available: ");
//             return true;
//         }
//         else {
//           Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
//         }
//     }
//     Serial.println("did not respond in time.");
//     return false;
// }





// // void SendToNodesTask(void *pvParameters) {
// //     while (true) {
// //         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for first message

// //         while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages before sleeping
// //             char *DataSend;
// //             if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
// //                 bool ack = waitForAck(&Serial2);

// //                 if (ack) {
// //                     Serial2.println(DataSend);
// //                     Serial.println("Sent JSON: \n" + String(DataSend));
// //                     free(DataSend);  // Free memory only when successfully sent
// //                 } else {
// //                     Serial.println("Ack not received for sending! Retrying...");
// //                     Serial.println("Did not send: " + String(DataSend));

// //                     // Push to retry queue if sending fails with a lower timeout
// //                     if (xQueueSend(retryQueue, &DataSend, 10 / portTICK_PERIOD_MS) != pdTRUE) {
// //                         Serial.println("Failed to send to retry queue. Message not requeued.");
// //                         free(DataSend);  // Free memory if failed to requeue
// //                     } else {
// //                         Serial.println("Message requeued to retry queue.");

// //                         if (uxQueueMessagesWaiting(retryQueue) > 0) {
// //                           char *retryData;
// //                           if (xQueueReceive(retryQueue, &retryData, portMAX_DELAY) == pdTRUE) {
// //                               // Try sending the message again
// //                               bool ack = waitForAck(&Serial2);

// //                               if (ack) {
// //                                   Serial2.println(retryData);
// //                                   Serial.println("Resent JSON: \n" + String(retryData));
// //                                   free(retryData);  // Free memory when successfully sent
// //                               } else {
// //                                   // If still fails, keep in retry queue for another attempt
// //                                   Serial.println("Resend failed, keeping in retry queue.");
// //                                   xQueueSend(retryQueue, &retryData, portMAX_DELAY);  // Reattempt later
// //                               }
// //                           }
// //                       }
// //                     }
// //                 }
// //             }
// //         }

// //         // Periodically check retryQueue for failed messages

// //     }
// // }





// Iteration 1
// #include <ArduinoJson.h>
// #include <Wire.h>
// // #include "SC16IS752.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"

// #define NODE_ID 1  // This node's ID
// #define ACK_TIMEOUT 500  // Timeout for acknowledgment (ms)
// #define LISTEN_TIMEOUT 500  
// #define WAITFORDATA_TIMEOUT 500  


// SemaphoreHandle_t serial1Semaphore;
// SemaphoreHandle_t serial2Semaphore;
// QueueHandle_t jsonQueue;
// QueueHandle_t retryQueue;

// // Task handles for managing FreeRTOS tasks
// TaskHandle_t Task1Handle = NULL;
// TaskHandle_t Task2Handle = NULL;
// TaskHandle_t Task3Handle = NULL;

// bool listenForPing(HardwareSerial *serialPort);
// String WaitForData(HardwareSerial &serial);
// String readFromSerial(HardwareSerial &serialPort);
// bool waitForAck(HardwareSerial *serialPort);
// String CreateJson();
// String createBatteryJson1();
// String createBatteryJson2();
// String createBatteryJson3();


// String Node_4_Data;
// bool Send_to_Node_4;


// void setup() {
//     Serial.begin(115200);
//     Serial2.begin(9600, SERIAL_8N1, 16, 17);  
//     Serial1.begin(9600, SERIAL_8N1, 4, 23);  

//     jsonQueue = xQueueCreate(10, sizeof(char *)); 
//     if (jsonQueue == NULL) {
//         Serial.println("Queue creation failed!");
//     } else {
//         Serial.println("Queue created successfully.");
//     }

//     retryQueue = xQueueCreate(10, sizeof(char *)); 
//     if (retryQueue == NULL) {
//         Serial.println("Retry Queue creation failed!");
//     } else {
//         Serial.println("Retry Queue created successfully.");
//     }


//     serial1Semaphore = xSemaphoreCreateBinary();
//     serial2Semaphore = xSemaphoreCreateBinary();
//     xSemaphoreGive(serial1Semaphore);
//     xSemaphoreGive(serial2Semaphore);

//     // Create FreeRTOS tasks
//     xTaskCreatePinnedToCore(listenForNodesTask, "listenForNodes", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0
//     xTaskCreatePinnedToCore(SendToNodesTask, "SendData", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
//     xTaskCreatePinnedToCore(RetrySendTask, "RetrySend", 4096, NULL, 1, &Task3Handle, 1); // Run on Core 1
// }

// void loop() {
//     // Empty: Tasks handle execution.
// }

// int i = 0;

// void listenForNodesTask(void *pvParameters) {
//     while (true) {
//         String tempData;

//         if (i == 0) {
//             tempData = createBatteryJson1();
//             i++;
//         } else if (i == 1) {
//             tempData = createBatteryJson2();
//             i++;
//         } else if (i == 2) {
//             tempData = createBatteryJson3();
//             i=0;
//         }

//         // Allocate memory dynamically
//         char *Data = strdup(tempData.c_str());

//         if (Data == NULL) {
//             Serial.println("Memory allocation failed!");
//             continue; // Skip sending if allocation failed
//         }

//         if (xQueueSend(jsonQueue, &Data, portMAX_DELAY) == pdPASS) {
//             if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
//                 xTaskNotifyGive(Task2Handle);
//             }
//         } else {
//             Serial.println("Queue full! Data not sent.");
//             free(Data);  // Prevent memory leak if sending fails
//         }

//         vTaskDelay(500 / portTICK_PERIOD_MS);
//     }
// }


// void SendToNodesTask(void *pvParameters) {

//     SemaphoreHandle_t semaphore;


//     semaphore = serial2Semaphore;

//     while (true) {
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing

//         while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages
//             char *DataSend;
//             if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
//                 if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
//                     bool ack = waitForAck(&Serial2);

//                     if (ack) {
//                         Serial2.println(DataSend);
//                         Serial.println("Sent JSON: \n" + String(DataSend));
//                         free(DataSend);
//                     } else {
//                         // Serial.println("Ack not received! Retrying...");
//                         Serial.println("Did not send: " + String(DataSend));

//                         if (xQueueSend(retryQueue, &DataSend, 10 / portTICK_PERIOD_MS) != pdTRUE) {
//                             Serial.println("Failed to send to retry queue. Message lost.");
//                             free(DataSend);
//                         } else {
//                             Serial.println("Message moved to retry queue.");
//                         }
//                     }
//                     xSemaphoreGive(semaphore);  // Release semaphore
//                     vTaskDelay(100 / portTICK_PERIOD_MS); 
//                 } 
//             }
//         }
//     }
// }

// void RetrySendTask(void *pvParameters) {

  
//     SemaphoreHandle_t semaphore;


//     semaphore = serial2Semaphore;

//     while (true) {
//         if (uxQueueMessagesWaiting(retryQueue) > 0) {  // Check if retry queue has messages
//             char *retryData;
//             if (xQueueReceive(retryQueue, &retryData, portMAX_DELAY) == pdTRUE) {
//                 if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
//                     bool ack = waitForAck(&Serial2);

//                     if (ack) {
//                         Serial2.println(retryData);
//                         Serial.println("Resent JSON: \n" + String(retryData));
//                         free(retryData);
//                     } else {
//                         Serial.println("Resend failed, keeping in retry queue.");
//                         xQueueSend(retryQueue, &retryData, portMAX_DELAY);
//                     }
//                     xSemaphoreGive(semaphore);  // Release semaphore
//                     vTaskDelay(100 / portTICK_PERIOD_MS); 
//                 } 
//             }
//         } 
//     }
// }


// String ReadFromSimulator() {
//     static String buffer = "";  // Static buffer to hold partial data between function calls
//     String jsonData = "";

//     // Keep reading the buffer until it's empty
//     while (Serial.available() > 0) {
//         char incomingChar = (char)Serial.read();  // Read one byte at a time
//         buffer += incomingChar;  // Append it to the buffer

//         // Check if we've received a complete line (i.e., contains a newline character)
//         if (incomingChar == '\n') {
//             jsonData = buffer;  // Set jsonData to the full message
//             buffer = "";  // Reset the buffer for the next message
//         }
//     }

//     // Return the complete message (if any) or an empty string
//     return jsonData;
// }


// String WaitForData(HardwareSerial &serial) {

//     String serial_number;   
//     String listen;
//     String receivedJsonSerial;
//     listen = "";

//     // Alternate between Serial1 and Serial2
//     if (&serial == &Serial2) {
//         serial_number = "serial2";
//     } else {
//         serial_number = "serial1";
//     }


//     unsigned long startTime = millis();  // Record the start time
//     while (true) {
//     listen = readFromSerial(serial); 
//     if (listen != "PING" && listen != "") {
//         Serial.println("Did not receive PING, instead received: " + listen + "from " + serial_number);
//         receivedJsonSerial = listen;
//         return receivedJsonSerial;
//     }

//     // Print status every time a PING is still being received
//     Serial.println("receiving" + listen + " PING after Available was sent to " + serial_number);

//     // Check if the 500 ms timeout has passed
//     if (millis() - startTime >= WAITFORDATA_TIMEOUT) {
//         Serial.println("Timeout reached, stopping PING loop.");
//         return "no data";
//     }

      
//   }
// }


    


// bool listenForPing(HardwareSerial *serialPort) {
//     String listen = "";
//     unsigned long startTime = millis();  
//     String serial_number;

//     if (serialPort == &Serial1) {
//         serial_number = "serial1";
//     } else if (serialPort == &Serial2) {
//         serial_number = "serial2";
//     } 

    
    
//     while (millis() - startTime < LISTEN_TIMEOUT) {  
//         listen = readFromSerial(*serialPort);  
//         listen.trim();  
        
//         Serial.println("Listening for PING from " + serial_number + ": [" + listen + "]");  

//         if (listen == "PING") {
//             Serial.println("Received PING");
//             serialPort->println("Available");  

            
//             return true;  
//         }

//         vTaskDelay(1 / portTICK_PERIOD_MS); 
//     }

//     Serial.println("Did not receive PING within timeout from " + serial_number);
        
    

//     return false;
// }




// String createBatteryJson1() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R001";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }


// String createBatteryJson2() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R002";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }



// String createBatteryJson3() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R003";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }




// String readFromSerial(HardwareSerial &serial) {


//     String receivedData = "";

//     // Take the semaphore before accessing the serial line
    
//     while (serial.available()) {
//         char c = serial.read();

//         if (c == '\n' || c == '\r') {
//             break;  // Stop reading at newline or carriage return
//         }

//         receivedData += c;
//         delay(2);  // Use delay to prevent task starvation
//     }
//     receivedData.trim();  // Remove unwanted spaces or newline chars


    

//     return receivedData;
// }


// String CreateJson() {
//     // Generate local JSON data
//     String jsonData;
//     StaticJsonDocument<200> localJson;
//     localJson["deviceID"] = 4;  
//     localJson["temperature"] = 25.4;  
//     localJson["voltage"] = 3.7;  

//     serializeJson(localJson, jsonData);
//     return jsonData;

// }

// bool waitForAck(HardwareSerial *serialPort) {

//     String serial_number;
//     String receivedData;

//     if (serialPort == &Serial1) {
//         serial_number = "serial1";
//     } else if (serialPort == &Serial2) {
//         serial_number = "serial2";
//     } 
    

//     unsigned long startTime = millis();
//     Serial.println("Sent: PING to " + serial_number);
//     while (millis() - startTime < ACK_TIMEOUT) {
//         serialPort->println("PING");
//         vTaskDelay(20 / portTICK_PERIOD_MS); 
//         String receivedData = readFromSerial(*serialPort);
//         if (receivedData == "Available") {
//             Serial.println("responded with available: ");
//             return true;
//         }
//         else {
//           Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
//         }
//     }
//     Serial.println("did not respond in time.");
//     return false;
// }





// // void SendToNodesTask(void *pvParameters) {
// //     while (true) {
// //         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for first message

// //         while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages before sleeping
// //             char *DataSend;
// //             if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
// //                 bool ack = waitForAck(&Serial2);

// //                 if (ack) {
// //                     Serial2.println(DataSend);
// //                     Serial.println("Sent JSON: \n" + String(DataSend));
// //                     free(DataSend);  // Free memory only when successfully sent
// //                 } else {
// //                     Serial.println("Ack not received for sending! Retrying...");
// //                     Serial.println("Did not send: " + String(DataSend));

// //                     // Push to retry queue if sending fails with a lower timeout
// //                     if (xQueueSend(retryQueue, &DataSend, 10 / portTICK_PERIOD_MS) != pdTRUE) {
// //                         Serial.println("Failed to send to retry queue. Message not requeued.");
// //                         free(DataSend);  // Free memory if failed to requeue
// //                     } else {
// //                         Serial.println("Message requeued to retry queue.");

// //                         if (uxQueueMessagesWaiting(retryQueue) > 0) {
// //                           char *retryData;
// //                           if (xQueueReceive(retryQueue, &retryData, portMAX_DELAY) == pdTRUE) {
// //                               // Try sending the message again
// //                               bool ack = waitForAck(&Serial2);

// //                               if (ack) {
// //                                   Serial2.println(retryData);
// //                                   Serial.println("Resent JSON: \n" + String(retryData));
// //                                   free(retryData);  // Free memory when successfully sent
// //                               } else {
// //                                   // If still fails, keep in retry queue for another attempt
// //                                   Serial.println("Resend failed, keeping in retry queue.");
// //                                   xQueueSend(retryQueue, &retryData, portMAX_DELAY);  // Reattempt later
// //                               }
// //                           }
// //                       }
// //                     }
// //                 }
// //             }
// //         }

// //         // Periodically check retryQueue for failed messages

// //     }
// // }





// Iteration 1
// #include <ArduinoJson.h>
// #include <Wire.h>
// // #include "SC16IS752.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"

// #define NODE_ID 1  // This node's ID
// #define ACK_TIMEOUT 500  // Timeout for acknowledgment (ms)
// #define LISTEN_TIMEOUT 500  
// #define WAITFORDATA_TIMEOUT 500  


// SemaphoreHandle_t serial1Semaphore;
// SemaphoreHandle_t serial2Semaphore;
// QueueHandle_t jsonQueue;
// QueueHandle_t retryQueue;

// // Task handles for managing FreeRTOS tasks
// TaskHandle_t Task1Handle = NULL;
// TaskHandle_t Task2Handle = NULL;
// TaskHandle_t Task3Handle = NULL;

// bool listenForPing(HardwareSerial *serialPort);
// String WaitForData(HardwareSerial &serial);
// String readFromSerial(HardwareSerial &serialPort);
// bool waitForAck(HardwareSerial *serialPort);
// String CreateJson();
// String createBatteryJson1();
// String createBatteryJson2();
// String createBatteryJson3();


// String Node_4_Data;
// bool Send_to_Node_4;


// void setup() {
//     Serial.begin(115200);
//     Serial2.begin(9600, SERIAL_8N1, 16, 17);  
//     Serial1.begin(9600, SERIAL_8N1, 4, 23);  

//     jsonQueue = xQueueCreate(10, sizeof(char *)); 
//     if (jsonQueue == NULL) {
//         Serial.println("Queue creation failed!");
//     } else {
//         Serial.println("Queue created successfully.");
//     }

//     retryQueue = xQueueCreate(10, sizeof(char *)); 
//     if (retryQueue == NULL) {
//         Serial.println("Retry Queue creation failed!");
//     } else {
//         Serial.println("Retry Queue created successfully.");
//     }


//     serial1Semaphore = xSemaphoreCreateBinary();
//     serial2Semaphore = xSemaphoreCreateBinary();
//     xSemaphoreGive(serial1Semaphore);
//     xSemaphoreGive(serial2Semaphore);

//     // Create FreeRTOS tasks
//     xTaskCreatePinnedToCore(listenForNodesTask, "listenForNodes", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0
//     xTaskCreatePinnedToCore(SendToNodesTask, "SendData", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
//     xTaskCreatePinnedToCore(RetrySendTask, "RetrySend", 4096, NULL, 1, &Task3Handle, 1); // Run on Core 1
// }

// void loop() {
//     // Empty: Tasks handle execution.
// }

// int i = 0;

// void listenForNodesTask(void *pvParameters) {
//     while (true) {
//         String tempData;

//         if (i == 0) {
//             tempData = createBatteryJson1();
//             i++;
//         } else if (i == 1) {
//             tempData = createBatteryJson2();
//             i++;
//         } else if (i == 2) {
//             tempData = createBatteryJson3();
//             i=0;
//         }

//         // Allocate memory dynamically
//         char *Data = strdup(tempData.c_str());

//         if (Data == NULL) {
//             Serial.println("Memory allocation failed!");
//             continue; // Skip sending if allocation failed
//         }

//         if (xQueueSend(jsonQueue, &Data, portMAX_DELAY) == pdPASS) {
//             if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
//                 xTaskNotifyGive(Task2Handle);
//             }
//         } else {
//             Serial.println("Queue full! Data not sent.");
//             free(Data);  // Prevent memory leak if sending fails
//         }

//         // TODO: Mariam, remove this after. Add a 5-second delay after processing all messages -Krystal
//         vTaskDelay(5000 / portTICK_PERIOD_MS);  // 5000 ms = 5 seconds
//         // vTaskDelay(500 / portTICK_PERIOD_MS);
//     }
// }


// void SendToNodesTask(void *pvParameters) {

//     SemaphoreHandle_t semaphore;


//     semaphore = serial2Semaphore;

//     while (true) {
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing

//         while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages
//             char *DataSend;
//             if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
//                 if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
//                     bool ack = waitForAck(&Serial2);

//                     if (ack) {
//                         Serial2.println(DataSend);
//                         Serial.println("Sent JSON: \n" + String(DataSend));
//                         free(DataSend);
//                     } else {
//                         // Serial.println("Ack not received! Retrying...");
//                         Serial.println("Did not send: " + String(DataSend));

//                         if (xQueueSend(retryQueue, &DataSend, 10 / portTICK_PERIOD_MS) != pdTRUE) {
//                             Serial.println("Failed to send to retry queue. Message lost.");
//                             free(DataSend);
//                         } else {
//                             Serial.println("Message moved to retry queue.");
//                         }
//                     }
//                     xSemaphoreGive(semaphore);  // Release semaphore
//                     vTaskDelay(100 / portTICK_PERIOD_MS); 
//                 } 
//             }
//         }
//         // TODO: Mariam, remove this after. Add a 5-second delay after processing all messages -Krystal
//         vTaskDelay(5000 / portTICK_PERIOD_MS);  // 5000 ms = 5 seconds
//     }
// }

// void RetrySendTask(void *pvParameters) {

  
//     SemaphoreHandle_t semaphore;


//     semaphore = serial2Semaphore;

//     while (true) {
//         if (uxQueueMessagesWaiting(retryQueue) > 0) {  // Check if retry queue has messages
//             char *retryData;
//             if (xQueueReceive(retryQueue, &retryData, portMAX_DELAY) == pdTRUE) {
//                 if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
//                     bool ack = waitForAck(&Serial2);

//                     if (ack) {
//                         Serial2.println(retryData);
//                         Serial.println("Resent JSON: \n" + String(retryData));
//                         free(retryData);
//                     } else {
//                         Serial.println("Resend failed, keeping in retry queue.");
//                         xQueueSend(retryQueue, &retryData, portMAX_DELAY);
//                     }
//                     xSemaphoreGive(semaphore);  // Release semaphore
//                     vTaskDelay(100 / portTICK_PERIOD_MS); 
//                 } 
//             }
//         } 
//     }
// }


// String ReadFromSimulator() {
//     static String buffer = "";  // Static buffer to hold partial data between function calls
//     String jsonData = "";

//     // Keep reading the buffer until it's empty
//     while (Serial.available() > 0) {
//         char incomingChar = (char)Serial.read();  // Read one byte at a time
//         buffer += incomingChar;  // Append it to the buffer

//         // Check if we've received a complete line (i.e., contains a newline character)
//         if (incomingChar == '\n') {
//             jsonData = buffer;  // Set jsonData to the full message
//             buffer = "";  // Reset the buffer for the next message
//         }
//     }

//     // Return the complete message (if any) or an empty string
//     return jsonData;
// }


// String WaitForData(HardwareSerial &serial) {

//     String serial_number;   
//     String listen;
//     String receivedJsonSerial;
//     listen = "";

//     // Alternate between Serial1 and Serial2
//     if (&serial == &Serial2) {
//         serial_number = "serial2";
//     } else {
//         serial_number = "serial1";
//     }


//     unsigned long startTime = millis();  // Record the start time
//     while (true) {
//     listen = readFromSerial(serial); 
//     if (listen != "PING" && listen != "") {
//         Serial.println("Did not receive PING, instead received: " + listen + "from " + serial_number);
//         receivedJsonSerial = listen;
//         return receivedJsonSerial;
//     }

//     // Print status every time a PING is still being received
//     Serial.println("receiving" + listen + " PING after Available was sent to " + serial_number);

//     // Check if the 500 ms timeout has passed
//     if (millis() - startTime >= WAITFORDATA_TIMEOUT) {
//         Serial.println("Timeout reached, stopping PING loop.");
//         return "no data";
//     }

      
//   }
// }


    


// bool listenForPing(HardwareSerial *serialPort) {
//     String listen = "";
//     unsigned long startTime = millis();  
//     String serial_number;

//     if (serialPort == &Serial1) {
//         serial_number = "serial1";
//     } else if (serialPort == &Serial2) {
//         serial_number = "serial2";
//     } 

    
    
//     while (millis() - startTime < LISTEN_TIMEOUT) {  
//         listen = readFromSerial(*serialPort);  
//         listen.trim();  
        
//         Serial.println("Listening for PING from " + serial_number + ": [" + listen + "]");  

//         if (listen == "PING") {
//             Serial.println("Received PING");
//             serialPort->println("Available");  

            
//             return true;  
//         }

//         vTaskDelay(1 / portTICK_PERIOD_MS); 
//     }

//     Serial.println("Did not receive PING within timeout from " + serial_number);
        
    

//     return false;
// }




// String createBatteryJson1() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R001";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }


// String createBatteryJson2() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R002";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }



// String createBatteryJson3() {
//     StaticJsonDocument<1024> doc;

//     doc["rack_id"] = "R003";
//     doc["module_id"] = "M001";
//     doc["deviceID"] = 1;

//     JsonArray cells = doc.createNestedArray("cells");
//     const char* ids[] = {"B001-R001-M001-C001", "B001-R001-M001-C002", "B001-R001-M001-C003", "B001-R001-M001-C004", "B001-R001-M001-C005", 
//                           "B001-R001-M001-C006", "B001-R001-M001-C007", "B001-R001-M001-C008", "B001-R001-M001-C009", "B001-R001-M001-C010"};
//     float voltages[] = {2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7, 2.7};
//     float currents[] = {4.579, 3.563, 3.844, 2.503, 4.396, 4.41, 4.64, 4.711, 4.122, 3.798};
//     float temperatures[] = {23.9, 25.369, 25.827, 25.546, 25.392, 24.6, 26.906, 25.594, 24.241, 24.834};
//     const char* statuses[] = {"Normal", "Compromised", "Normal", "Compromised", "Compromised", "Normal", "Normal", "Compromised", "Normal", "Normal"};
//     const char* faults[] = {"Normal", "Over_current", "Normal", "Overheating", "Over_discharge", "Normal", "Normal", "Over_current", "Normal", "Normal"};

//     for (int i = 0; i < 10; i++) {
//         JsonObject cell = cells.createNestedObject();
//         cell["id"] = ids[i];
//         cell["voltage"] = voltages[i];
//         cell["current"] = currents[i];
//         cell["temperature"] = temperatures[i];
//         cell["status"] = statuses[i];
//         cell["faults"] = faults[i];
//     }

//     String output;
//     serializeJson(doc, output);
//     return output;
// }




// String readFromSerial(HardwareSerial &serial) {


//     String receivedData = "";

//     // Take the semaphore before accessing the serial line
    
//     while (serial.available()) {
//         char c = serial.read();

//         if (c == '\n' || c == '\r') {
//             break;  // Stop reading at newline or carriage return
//         }

//         receivedData += c;
//         delay(2);  // Use delay to prevent task starvation
//     }
//     receivedData.trim();  // Remove unwanted spaces or newline chars


    

//     return receivedData;
// }


// String CreateJson() {
//     // Generate local JSON data
//     String jsonData;
//     StaticJsonDocument<200> localJson;
//     localJson["deviceID"] = 4;  
//     localJson["temperature"] = 25.4;  
//     localJson["voltage"] = 3.7;  

//     serializeJson(localJson, jsonData);
//     return jsonData;

// }

// bool waitForAck(HardwareSerial *serialPort) {

//     String serial_number;
//     String receivedData;

//     if (serialPort == &Serial1) {
//         serial_number = "serial1";
//     } else if (serialPort == &Serial2) {
//         serial_number = "serial2";
//     } 
    

//     unsigned long startTime = millis();
//     Serial.println("Sent: PING to " + serial_number);
//     while (millis() - startTime < ACK_TIMEOUT) {
//         serialPort->println("PING");
//         vTaskDelay(20 / portTICK_PERIOD_MS); 
//         String receivedData = readFromSerial(*serialPort);
//         if (receivedData == "Available") {
//             Serial.println("responded with available: ");
//             return true;
//         }
//         else {
//           Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
//         }
//     }
//     Serial.println("did not respond in time.");
//     return false;
// }





// // void SendToNodesTask(void *pvParameters) {
// //     while (true) {
// //         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for first message

// //         while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages before sleeping
// //             char *DataSend;
// //             if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
// //                 bool ack = waitForAck(&Serial2);

// //                 if (ack) {
// //                     Serial2.println(DataSend);
// //                     Serial.println("Sent JSON: \n" + String(DataSend));
// //                     free(DataSend);  // Free memory only when successfully sent
// //                 } else {
// //                     Serial.println("Ack not received for sending! Retrying...");
// //                     Serial.println("Did not send: " + String(DataSend));

// //                     // Push to retry queue if sending fails with a lower timeout
// //                     if (xQueueSend(retryQueue, &DataSend, 10 / portTICK_PERIOD_MS) != pdTRUE) {
// //                         Serial.println("Failed to send to retry queue. Message not requeued.");
// //                         free(DataSend);  // Free memory if failed to requeue
// //                     } else {
// //                         Serial.println("Message requeued to retry queue.");

// //                         if (uxQueueMessagesWaiting(retryQueue) > 0) {
// //                           char *retryData;
// //                           if (xQueueReceive(retryQueue, &retryData, portMAX_DELAY) == pdTRUE) {
// //                               // Try sending the message again
// //                               bool ack = waitForAck(&Serial2);

// //                               if (ack) {
// //                                   Serial2.println(retryData);
// //                                   Serial.println("Resent JSON: \n" + String(retryData));
// //                                   free(retryData);  // Free memory when successfully sent
// //                               } else {
// //                                   // If still fails, keep in retry queue for another attempt
// //                                   Serial.println("Resend failed, keeping in retry queue.");
// //                                   xQueueSend(retryQueue, &retryData, portMAX_DELAY);  // Reattempt later
// //                               }
// //                           }
// //                       }
// //                     }
// //                 }
// //             }
// //         }

// //         // Periodically check retryQueue for failed messages

// //     }
// // }
