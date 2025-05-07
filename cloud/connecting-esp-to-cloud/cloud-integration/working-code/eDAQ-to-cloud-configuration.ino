#include <ArduinoJson.h>
#include <Wire.h>
#include "SC16IS752.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <map>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "time.h"

#define NODE_ID 5  // This node's ID
#define ACK_TIMEOUT 500  // Timeout for acknowledgment (ms)
#define LISTEN_TIMEOUT 500  
#define WAITFORDATA_TIMEOUT 500  
/ AWS IoT Topics
// TODO: Fix the topic when the code is functional
#define AWS_IOT_PUBLISH_TOPIC "esp/Module/bulk"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;         // Standard Time
const int daylightOffset_sec = 3600;          // Add 3600 during DST
StaticJsonDocument<3072> doc;                 // TOOD: Change the size of the document if Mariam snd a different size JSON object


// Global Variables
WiFiClientSecure net;
MQTTClient client(256);


// Define destination node numbers
#define NODE_RIGHT 9
#define NODE_DOWN 6
#define NODE_LEFT


#define SEND_QUEUE_NAME(node) send_node_##node##Queue
#define RETRY_QUEUE_NAME(node) retry_node_##node##Queue

// Semaphore and Queue Definitions
SemaphoreHandle_t serial1Semaphore;
SemaphoreHandle_t serial2Semaphore;
QueueHandle_t jsonQueue;
QueueHandle_t processing_Queue;

// Automatically create send & retry queues for defined nodes
QueueHandle_t SEND_QUEUE_NAME(NODE_RIGHT);  
QueueHandle_t SEND_QUEUE_NAME(NODE_DOWN);  
QueueHandle_t RETRY_QUEUE_NAME(NODE_RIGHT); 
QueueHandle_t RETRY_QUEUE_NAME(NODE_DOWN); 
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

    RETRY_QUEUE_NAME(NODE_RIGHT) = xQueueCreate(10, sizeof(char *)); 
    if ( RETRY_QUEUE_NAME(NODE_RIGHT) == NULL) {
        Serial.println("retry node right  Queue creation failed!");
    } else {
        Serial.println("retry node right queue created successfully.");
    }


    RETRY_QUEUE_NAME(NODE_DOWN) = xQueueCreate(10, sizeof(char *)); 
    if (RETRY_QUEUE_NAME(NODE_DOWN) == NULL) {
        Serial.println("down node queue creation failed!");
    } else {
        Serial.println("down node queue created successfully.");
    }

    SEND_QUEUE_NAME(NODE_RIGHT) = xQueueCreate(10, sizeof(char *)); 
    if (SEND_QUEUE_NAME(NODE_RIGHT) == NULL) {
        Serial.println("right node queue creation failed!");
    } else {
        Serial.println("right node queue created successfully.");
    }


    SEND_QUEUE_NAME(NODE_DOWN) = xQueueCreate(10, sizeof(char *)); 
    if (SEND_QUEUE_NAME(NODE_DOWN)== NULL) {
        Serial.println("Node down queue creation failed!");
    } else {
        Serial.println("Node down queue created successfully.");
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
    // xTaskCreatePinnedToCore(listenForNodesTask, "listenForLeftNode", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0t
    xTaskCreatePinnedToCore(listenForLeftNodeTask, "listenForNode2", 4096, NULL, 1, &Task4Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(ParseTask, "Parse", 4096, NULL, 1, &Task5Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(SendToRightNodeTask, "SendData", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(RetrySendRightNodeTask, "RetrySend", 4096, NULL, 1, &Task3Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(SendToDownNodeTask, "SendData", 4096, NULL, 1, &Task6Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(RetrySendDownNodeTask, "RetrySend", 4096, NULL, 1, &Task7Handle, 1); // Run on Core 1
    xTaskCreatePinnedToCore(ProcessTask, "ProcesData", 4096, NULL, 1, &Task8Handle, 1); // Run on Core 1
}

void loop() {
    // Empty: Tasks handle execution.
}

int i = 0;

void listenForLeftNodeTask(void *pvParameters) {
    
    SemaphoreHandle_t *semaphore;

    String tempData;
    int ping;

    String serial_number;

    HardwareSerial *serialPort;

    serialPort = &Serial2;
    semaphore = &serial2Semaphore;
    serial_number = "serial2";
    while (true) {

    
    // Serial.println("listen to node 1 starts");



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

        if (xQueueSend(jsonQueue, &Data, 50 / portTICK_PERIOD_MS) == pdPASS) {
            if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
                
                xTaskNotifyGive(Task5Handle);
            }
        } else {
            Serial.println("jsonQueue full! Data not sent.");
            free(Data);  // Prevent memory leak if sending fails
        }
      }




    }


      vTaskDelay(50 / portTICK_PERIOD_MS);
        
}


void listenForNode2Task(void *pvParameters) {

  
    SemaphoreHandle_t *semaphore;

    String tempData;
    int ping;

    String serial_number;

    HardwareSerial *serialPort;

    serialPort = &Serial2;
    semaphore = &serial2Semaphore;
    serial_number = "serial2";
    while (true) {

    



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

        if (xQueueSend(jsonQueue, &Data, 50 / portTICK_PERIOD_MS) == pdPASS) {
            // Serial.println("added to node right queue");
            if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
                // Serial.println("notifying parsing task");
                xTaskNotifyGive(Task5Handle); // parse
            }
        } else {
            Serial.println("send node right queue full! Data not sent.");
            free(Data);  
        }
      }




    }


      vTaskDelay(100 / portTICK_PERIOD_MS);
        
  }


void SendToRightNodeTask(void *pvParameters) {

     SemaphoreHandle_t semaphore;

    semaphore = serial2Semaphore;

    while (true) {
        Serial.println("reached task");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing
        Serial.println("Reached after task");

        while (uxQueueMessagesWaiting(SEND_QUEUE_NAME(NODE_RIGHT)) > 0) {  // Process all messages
            char *DataSend;
            if (xQueueReceive(SEND_QUEUE_NAME(NODE_RIGHT), &DataSend, portMAX_DELAY) == pdTRUE) {
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


void SendToDownNodeTask(void *pvParameters) {

    SemaphoreHandle_t semaphore;


    semaphore = serial1Semaphore;

    HardwareSerial &serial = Serial1;

    String serial_name = "Serial1";
    

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing

        while (uxQueueMessagesWaiting(SEND_QUEUE_NAME(NODE_DOWN)) > 0) {  // Process all messages
            char *DataSend;
            if (xQueueReceive(SEND_QUEUE_NAME(NODE_DOWN), &DataSend, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
                    char *DataCopy = strdup(DataSend);
                    if (DataCopy == NULL) {
                        Serial.println("Memory allocation failed for datacopy in send to right node task!");
                        free(DataSend); 
                        continue;   
                    }
                    bool ack = waitForAck(&serial);

                    if (ack) {
                        Serial2.println(DataSend);
                        Serial.println("Sent JSON to " + serial_name+ ": \n" + String(DataSend));
                        
                    } else {
                        // Serial.println("Ack not received! Retrying...");
                        Serial.println("Did not send: " + String(DataSend));

                        if (xQueueSend(RETRY_QUEUE_NAME(NODE_DOWN), &DataCopy, 50 / portTICK_PERIOD_MS) != pdTRUE) {
                            Serial.println("Failed to send to retry queue for down node. Message lost meant for " + serial_name);
                            free(DataCopy);
                        } else {
                            Serial.println("Message moved to retry queue after attempting to send to " + serial_name);
                        }
                    }
                    free(DataSend);
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
            }
        }
    }
}

void RetrySendRightNodeTask(void *pvParameters) {

  
    SemaphoreHandle_t semaphore;


    semaphore = serial1Semaphore;
    
    HardwareSerial &serial = Serial1;
    String serial_name = "Serial1";

    while (true) {
        if (uxQueueMessagesWaiting(RETRY_QUEUE_NAME(NODE_RIGHT)) > 0) {  // Check if retry queue has messages
            char *retryData;
            if (xQueueReceive(RETRY_QUEUE_NAME(NODE_RIGHT), &retryData, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
                    char *DataCopy = strdup(retryData);
                    if (DataCopy == NULL) {
                        Serial.println("Memory allocation failed for datacopy in send to retry right node task!");
                        free(retryData); 
                        continue;   
                    }
                    bool ack = waitForAck(&serial);

                    if (ack) {
                        Serial2.println(retryData);
                        Serial.println("Resent JSON to " + serial_name+ "\n" + String(retryData));
                        // free(retryData);
                    } else {
                        Serial.println("Resend failed to " + serial_name+ ", sending to node down queue");
                        xQueueSend(SEND_QUEUE_NAME(NODE_DOWN), &retryData, 50 / portTICK_PERIOD_MS);
                    }
                    free(retryData);
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
            }
        } 
    }
}


void RetrySendDownNodeTask(void *pvParameters) {

  
    SemaphoreHandle_t semaphore;


    semaphore = serial1Semaphore;
    
    HardwareSerial &serial = Serial1;
    String serial_name = "Serial1";

    while (true) {
        if (uxQueueMessagesWaiting(RETRY_QUEUE_NAME(NODE_DOWN)) > 0) {  // Check if retry queue has messages
            char *retryData;
            if (xQueueReceive(RETRY_QUEUE_NAME(NODE_DOWN), &retryData, portMAX_DELAY) == pdTRUE) {
                if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore
                    bool ack = waitForAck(&serial);

                    if (ack) {
                        Serial2.println(retryData);
                        Serial.println("Resent JSON to " + serial_name+ "\n" + String(retryData));
                        free(retryData);
                    } else {
                        Serial.println("Resend failed to " + serial_name+ ", keeping in retry queue.");
                        xQueueSend(RETRY_QUEUE_NAME(NODE_DOWN), &retryData, 50 / portTICK_PERIOD_MS);
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
        // Serial.println("entered parse task");

        if (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Check if JSON queue has messages
            // Serial.println("parsing queue");
            char *Data;
            if (xQueueReceive(jsonQueue, &Data, portMAX_DELAY) == pdTRUE) {
                // Serial.println("found data in queue");
                int deviceID = DeviceID(Data);
                if (deviceID == NODE_ID) {
                    // Try adding to processing queue
                    if (xQueueSend(processing_Queue, &Data, 50 / portTICK_PERIOD_MS) == pdTRUE) {

                        Serial.println("Sent to processing, device id matches");
                        xTaskNotifyGive(Task8Handle); // process it

                    } 
                    else {
                        Serial.println("Processing queue full");
                    }
                }
                else {
                        String message = String(Data);
                        Serial.println("device id does not match, sending right"); 
                        xQueueSend(SEND_QUEUE_NAME(NODE_RIGHT), &Data, 50 / portTICK_PERIOD_MS);
                        xTaskNotifyGive(Task2Handle); // send to node 5
                }
                // free(Data);
            }
        } 
    }
}



// void ProcessTask(void *pvParameters) {
//     while (true) {
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//         if (uxQueueMessagesWaiting(processing_Queue) > 0) {
//             char *Data;
//             if (xQueueReceive(processing_Queue, &Data, portMAX_DELAY) == pdTRUE) {
//               StaticJsonDocument<1024> doc;
//               DeserializationError error = deserializeJson(doc, Data);

//               if (error) {
//                   Serial.print("JSON Parse Error: ");
//                   Serial.println(error.c_str());
                  
//               }

//               JsonArray cells = doc["cells"];
//               float totalVoltage = 0, totalCurrent = 0, totalTemperature = 0;
//               const char* faultList[10];

//               for (int i = 0; i < cells.size(); i++) {
//                   totalVoltage += cells[i]["voltage"].as<float>();
//                   totalCurrent += cells[i]["current"].as<float>();
//                   totalTemperature += cells[i]["temperature"].as<float>();
//                   faultList[i] = cells[i]["faults"].as<const char*>();
//               }

//               float avgVoltage = totalVoltage / cells.size();
//               float avgCurrent = totalCurrent / cells.size();
//               float avgTemperature = totalTemperature / cells.size();
//               const char* commonFault = mostCommonFault(faultList, cells.size());

//               // Create new JSON
//               StaticJsonDocument<256> summaryDoc;
//               summaryDoc["rack_id"] = doc["rack_id"];
//               summaryDoc["module_id"] = doc["module_id"];
//               summaryDoc["deviceID"] = doc["deviceID"];
//               summaryDoc["avg_voltage"] = avgVoltage;
//               summaryDoc["avg_current"] = avgCurrent;
//               summaryDoc["avg_temperature"] = avgTemperature;
//               summaryDoc["most_common_fault"] = commonFault;

//               String output;
//               serializeJson(summaryDoc, output);

//               // Copy output into a dynamically allocated char array for queue safety
//               char* jsonCopy = (char*)pvPortMalloc(output.length() + 1);
//               if (jsonCopy != NULL) {
//                   strcpy(jsonCopy, output.c_str());

//                   if (xQueueSend(SEND_QUEUE_NAME(NODE_RIGHT), &jsonCopy, 50 / portTICK_PERIOD_MS) == pdPASS) {
//                       if (uxQueueMessagesWaiting(SEND_QUEUE_NAME(NODE_RIGHT)) == 1) {  // Notify only for first message
//                           xTaskNotifyGive(Task2Handle); // Notify send_to_node_5 task
//                       }
//                   } else {
//                       Serial.println("send node right full! Data not sent.");
//                       vPortFree(jsonCopy);  // Prevent memory leak
//                   }
//               } else {
//                   Serial.println("Memory allocation failed!");
//               }

//               Serial.println("Processed JSON: \n" + output);
//               free(Data);
//           }
//         }
//     }
// }


// void ProcessTask(void *pvParameters) {
//     while (true) {
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//         if (uxQueueMessagesWaiting(processing_Queue) > 0) {
//             char *Data;
//             if (xQueueReceive(processing_Queue, &Data, portMAX_DELAY) == pdTRUE) {
//                 StaticJsonDocument<2048> doc;
//                 DeserializationError error = deserializeJson(doc, Data);
//                 if (error) {
//                     Serial.print("JSON Parse Error: ");
//                     Serial.println(error.c_str());
//                 }

//                 JsonArray cells = doc["cells"];
//                 float totalVoltage = 0, totalCurrent = 0, totalTemperature = 0;
//                 int totalCells = cells.size();
//                 int normalCells = 0, compromisedCells = 0;
//                 std::map<String, int> faultCount;
//                 JsonArray compromisedArray;

//                 StaticJsonDocument<1024> summaryDoc;
//                 summaryDoc["timeInSeconds"] = String(millis() / 1000);
//                 summaryDoc["moduleID"] = doc["module_id"];
//                 summaryDoc["deviceID"] = doc["deviceID"];
//                 JsonArray compromisedList = summaryDoc.createNestedArray("compromised_cells");

//                 for (JsonObject cell : cells) {
//                     float voltage = cell["voltage"].as<float>();
//                     float current = cell["current"].as<float>();
//                     float temperature = cell["temperature"].as<float>();
//                     const char* status = cell["status"];
//                     const char* fault = cell["faults"];
                    
//                     totalVoltage += voltage;
//                     totalCurrent += current;
//                     totalTemperature += temperature;
//                     faultCount[fault]++;

//                     if (String(status) == "Normal") {
//                         normalCells++;
//                     } else {
//                         compromisedCells++;
//                         JsonObject compromisedCell = compromisedList.createNestedObject();
//                         compromisedCell["id"] = cell["id"];
//                         compromisedCell["voltage"] = voltage;
//                         compromisedCell["curr"] = current;
//                         compromisedCell["temperature"] = temperature;
//                         compromisedCell["status"] = status;
//                         compromisedCell["faults"] = fault;
//                     }
//                 }

//                 JsonObject stats = summaryDoc.createNestedObject("statistics");
//                 stats["total_cells"] = totalCells;
//                 stats["normal_cells"] = normalCells;
//                 stats["compromised_cells"] = compromisedCells;
//                 stats["average_current"] = totalCurrent / totalCells;
//                 stats["average_voltage"] = totalVoltage / totalCells;
//                 stats["average_temperature"] = totalTemperature / totalCells;
//                 JsonObject faultCounts = stats.createNestedObject("fault_counts");

//                 for (auto &entry : faultCount) {
//                     faultCounts[entry.first] = entry.second;
//                 }

//                 String output;
//                 serializeJson(summaryDoc, output);

//                 char* jsonCopy = (char*)pvPortMalloc(output.length() + 1);
//                 if (jsonCopy != NULL) {
//                     strcpy(jsonCopy, output.c_str());
//                     if (xQueueSend(SEND_QUEUE_NAME(NODE_RIGHT), &jsonCopy, 50 / portTICK_PERIOD_MS) == pdPASS) {
//                         if (uxQueueMessagesWaiting(SEND_QUEUE_NAME(NODE_RIGHT)) == 1) {
//                             xTaskNotifyGive(Task2Handle);
//                         }
//                     } else {
//                         Serial.println("send node right full! Data not sent.");
//                         vPortFree(jsonCopy);
//                     }
//                 } else {
//                     Serial.println("Memory allocation failed!");
//                 }
//                 Serial.println("Processed JSON: \n" + output);
//                 free(Data);
//             }
//         }
//     }
// }


void ProcessTask(void *pvParameters) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (uxQueueMessagesWaiting(processing_Queue) > 0) {
            char *Data;
            if (xQueueReceive(processing_Queue, &Data, portMAX_DELAY) == pdTRUE) {
                StaticJsonDocument<2048> doc;
                DeserializationError error = deserializeJson(doc, Data);
                if (error) {
                    Serial.print("JSON Parse Error: ");
                    Serial.println(error.c_str());
                }

                JsonArray cells = doc["cells"];
                float totalVoltage = 0, totalCurrent = 0, totalTemperature = 0;
                int totalCells = cells.size();
                int normalCells = 0, compromisedCells = 0;
                std::map<String, int> faultCount;
                JsonArray compromisedArray;

                StaticJsonDocument<1024> summaryDoc;
                summaryDoc["timeInSeconds"] = String(millis() / 1000);
                summaryDoc["moduleID"] = doc["module_id"];
                summaryDoc["deviceID"] = doc["deviceID"];
                
                // Create statistics before compromised_cells
                JsonObject stats = summaryDoc.createNestedObject("statistics");
                JsonArray compromisedList = summaryDoc.createNestedArray("compromised_cells");

                for (JsonObject cell : cells) {
                    float voltage = cell["voltage"].as<float>();
                    float current = cell["current"].as<float>();
                    float temperature = cell["temperature"].as<float>();
                    const char* status = cell["status"];
                    const char* fault = cell["faults"];
                    
                    totalVoltage += voltage;
                    totalCurrent += current;
                    totalTemperature += temperature;
                    faultCount[fault]++;

                    if (String(status) == "Normal") {
                        normalCells++;
                    } else {
                        compromisedCells++;
                        JsonObject compromisedCell = compromisedList.createNestedObject();
                        compromisedCell["id"] = cell["id"];
                        compromisedCell["voltage"] = voltage;
                        compromisedCell["curr"] = current;
                        compromisedCell["temperature"] = temperature;
                        compromisedCell["status"] = status;
                        compromisedCell["faults"] = fault;
                    }
                }

                // Populate statistics
                stats["total_cells"] = totalCells;
                stats["normal_cells"] = normalCells;
                stats["compromised_cells"] = compromisedCells;
                stats["average_current"] = totalCurrent / totalCells;
                stats["average_voltage"] = totalVoltage / totalCells;
                stats["average_temperature"] = totalTemperature / totalCells;
                
                // Populate fault counts
                JsonObject faultCounts = stats.createNestedObject("fault_counts");
                for (auto &entry : faultCount) {
                    faultCounts[entry.first] = entry.second;
                }

                String output;
                serializeJson(summaryDoc, output);

                // Allocate memory for the output JSON string
                char* jsonCopy = (char*)pvPortMalloc(output.length() + 1);
                if (jsonCopy != NULL) {
                    strcpy(jsonCopy, output.c_str());

                    // Send JSON to the next queue
                    if (xQueueSend(SEND_QUEUE_NAME(NODE_RIGHT), &jsonCopy, 50 / portTICK_PERIOD_MS) == pdPASS) {
                        if (uxQueueMessagesWaiting(SEND_QUEUE_NAME(NODE_RIGHT)) == 1) {
                            xTaskNotifyGive(Task2Handle);  // Notify Task2 to send data
                        }
                    } else {
                        Serial.println("send node right full! Data not sent.");
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
    // Serial.println("receiving" + listen + " PING after Available was sent to " + serial_number);

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
        
        // Serial.println("Listening for PING from " + serial_number + ": [" + listen + "]");  

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
          // Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
        }
    }
    Serial.println("did not respond in time.");
    return false;
}

String generateRandomID() {
    char idBuffer[20];
    snprintf(idBuffer, sizeof(idBuffer), "B%03d-R%03d-M%03d-C%03d",
             random(1, 1000), random(1, 1000), random(1, 1000), random(1, 1000));
    return String(idBuffer);
}

// Function to connect ESP32 to Wi-Fi
void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
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
        delay(1000);  // Retry every second

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

// Function to connect ESP32 to AWS IoT
// void connectAWS() {
//     // Load AWS Certificates
//     net.setCACert(AWS_CERT_CA);
//     net.setCertificate(AWS_CERT_CRT);
//     net.setPrivateKey(AWS_CERT_PRIVATE);

//     // Configure MQTT Client
//     client.begin(AWS_IOT_ENDPOINT, 8883, net);

//     Serial.print("Connecting to AWS IoT...");
//     unsigned long startAttemptTime = millis();
//     const unsigned long connectionTimeout = 10000; // 10 seconds timeout

//     while (!client.connect(THINGNAME)) {
//         Serial.print(".");
//         delay(2000); // Wait 2 seconds before retrying

//         if (millis() - startAttemptTime > connectionTimeout) {
//             Serial.println("\nAWS IoT Connection Timeout!");
//             Serial.print("MQTT Error Code: ");
//             Serial.println(client.lastError());
//             return;
//         }
//     }

//     Serial.println("\nConnected to AWS IoT!");

//     // Subscribe to a topic
//     Serial.print("Subscribing to topic: ");
//     Serial.println(AWS_IOT_PUBLISH_TOPIC);
//     client.subscribe(AWS_IOT_PUBLISH_TOPIC);
// }

// Function to randomly generate the JSON packet that will be published to a topic in the cloud
// returns the JSON object.
// TODO: Fix what the device sends
StaticJsonDocument<3072> generateMessage() {
    StaticJsonDocument<3072> doc;

    // Identifiers
    doc["timeInSeconds"] = String(time(nullptr));
    doc["moduleID"] = "M009";
    doc["deviceID"] = random(1, 11);

    // Statistics + Compromised array
    JsonObject stats = doc.createNestedObject("statistics");
    JsonArray compromised = doc.createNestedArray("compromised_cells");

    // STEP 1: Total cells and guaranteed ≥ 1 compromised
    int total_cells = random(1, 11); // 1–10
    int max_normal = max(0, total_cells - 1); // Leave space for 1 compromised
    int normal_cells = constrain(random(4, 8), 0, max_normal);
    int compromised_cells = total_cells - normal_cells;

    // STEP 2: Fault distribution (must add to total_cells)
    int fault_normal = 0, fault_oc = 0, fault_oh = 0, fault_od = 0;
    int remaining = total_cells;

    fault_normal = min((int)random(4, 8), remaining);
    remaining -= fault_normal;
    if (remaining > 0) { fault_oc = random(0, remaining + 1); remaining -= fault_oc; }
    if (remaining > 0) { fault_oh = random(0, remaining + 1); remaining -= fault_oh; }
    if (remaining > 0) { fault_od = remaining; }

    // STEP 3: Generate compromised cell values
    float sum_voltage = 0.0, sum_current = 0.0, sum_temp = 0.0;

    for (int i = 0; i < compromised_cells; i++) {
        float voltage = 2.7;
        float current = random(200, 500) / 100.0;
        float temp = random(2300, 2700) / 100.0;

        sum_voltage += voltage;
        sum_current += current;
        sum_temp += temp;

        String fault;
        if (fault_oc > 0) { fault = "Over_current"; fault_oc--; }
        else if (fault_oh > 0) { fault = "Overheating"; fault_oh--; }
        else if (fault_od > 0) { fault = "Over_discharge"; fault_od--; }
        else { fault = "Normal"; fault_normal--; }

        JsonObject cell = compromised.createNestedObject();
        cell["id"] = generateRandomID();
        cell["voltage"] = voltage;
        cell["curr"] = current;
        cell["temperature"] = temp;
        cell["status"] = "Compromised";
        cell["faults"] = fault;
    }

    // STEP 4: Stats block
    stats["total_cells"] = total_cells;
    stats["normal_cells"] = normal_cells;
    stats["compromised_cells"] = compromised_cells;
    stats["average_voltage"] = compromised_cells > 0 ? sum_voltage / compromised_cells : 0.0;
    stats["average_current"] = compromised_cells > 0 ? sum_current / compromised_cells : 0.0;
    stats["average_temperature"] = compromised_cells > 0 ? sum_temp / compromised_cells : 0.0;

    JsonObject fault_counts = stats.createNestedObject("fault_counts");
    fault_counts["Normal"] = fault_normal;
    fault_counts["Over_current"] = fault_oc;
    fault_counts["Overheating"] = fault_oh;
    fault_counts["Over_discharge"] = fault_od;

    return doc;
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

// Improved setup to handle connections better
void setup() {
    Serial.begin(115200);

    // Connect to Wi-Fi
    connectWiFi();

    // Sync time for certificate validity
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        delay(20000);
    }

    // Initial connection to AWS IoT
    connectAWS();

    // Set MQTT message callback
    client.onMessage(messageHandler);
}


