
#include <ArduinoJson.h>
#include <Wire.h>
// #include "SC16IS752.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <map>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include "WiFi.h"
#include "time.h"

#define NODE_ID 13  // This node's ID
#define ACK_TIMEOUT 500  // Timeout for acknowledgment (ms)
#define LISTEN_TIMEOUT 500  
#define WAITFORDATA_TIMEOUT 500  



// AWS IoT Topics
// TODO: Mariam, if we sent data from 4 exits nodes, we will have to upload different code to 4 ESP32s
        // the only thing that changes is "esp/Module/bulk"
        // we would have ot make it ""esp/Module/bulk#"", where "#" is 1-4, and setup on4 ESP32s.
#define AWS_IOT_PUBLISH_TOPIC "esp/Module/bulk1"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;         // Standard Time
const int daylightOffset_sec = 3600;          // Add 3600 during DST
StaticJsonDocument<3072> doc;                 // TOOD: Change the size of the document if Mariam snd a different size JSON object


// Global Variables
WiFiClientSecure net;
MQTTClient client(2048);

#define CH_A SC16IS752_CHANNEL_A
#define CH_B SC16IS752_CHANNEL_B

// SC16IS752 i2cuart = SC16IS752(SC16IS750_PROTOCOL_I2C,SC16IS750_ADDRESS_BB); 

// Define destination node numbers
#define NODE_RIGHT 9
#define NODE_DOWN 6
#define NODE_LEFT 1


#define SEND_QUEUE_NAME(node) send_node_##node##Queue
#define RETRY_QUEUE_NAME(node) retry_node_##node##Queue

SemaphoreHandle_t serial1Semaphore;
SemaphoreHandle_t serial2Semaphore;
SemaphoreHandle_t CHA_semaphore;
QueueHandle_t jsonQueue;
QueueHandle_t cloud_Queue;
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
String createBatteryJson4();
int DeviceID(String json);
const char* mostCommonFault(const char* faults[], int size);
bool WaitForAckExtender(uint8_t channel);
bool ListenForPingExtender(uint8_t channel);
String WaitForDataExtender(uint8_t channel);
String ReadFromExtender(uint8_t channel);
void EncodeString(const char *input, uint8_t *output, uint8_t length);
void WriteFromExtender(uint8_t channel, const char *input);



void setup() {

    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);  
    Serial1.begin(9600, SERIAL_8N1, 4, 23);  


    jsonQueue = xQueueCreate(10, sizeof(char *)); 
    if (jsonQueue == NULL) {
        Serial.println("Queue creation failed!");
    } else {
        Serial.println("Queue created successfully.");
    }

    cloud_Queue = xQueueCreate(10, sizeof(char *)); 
    if (cloud_Queue == NULL) {
        Serial.println("cloud_Queue creation failed!");
    } else {
        Serial.println("cloud_Queue created successfully.");
    }

    processing_Queue = xQueueCreate(10, sizeof(char *)); 
    if (processing_Queue == NULL) {
        Serial.println("processing_Queue creation failed!");
    } else {
        Serial.println("processing_Queue created successfully.");
    }

    connectWiFi();

    // Sync time for certificate validity
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        // delay(20000);
    }

    // Initial connection to AWS IoT
    connectAWS();

    // Set MQTT message callback
    client.onMessage(messageHandler);




    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(listenForLeftNodeTask, "listenForNode5", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0t
    xTaskCreatePinnedToCore(ParseTask, "Parse", 4096, NULL, 1, &Task5Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(SendToCloud, "sendtocloud", 8192, NULL, 1, &Task2Handle, 1); 
    xTaskCreatePinnedToCore(ProcessTask, "ProcesData", 4096, NULL, 1, &Task8Handle, 0); // Run on Core 1
}

void loop() {
    // Empty: Tasks handle execution.
}


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
        if (tempData == "no data"){
        Serial.println("nodata");
        continue;
      }

      Serial.println("Received Data (should be JSON) = " + tempData);


        char *Data = strdup(tempData.c_str());

        if (Data == NULL) {
            Serial.println("Memory allocation failed for listen for left node task!");
            free(Data);
            continue; // Skip sending if allocation failed
        }

        
        if (xQueueSend(jsonQueue, &Data, 50 / portTICK_PERIOD_MS) == pdPASS) {
            if (uxQueueMessagesWaiting(jsonQueue) == 1) {
                xTaskNotifyGive(Task5Handle);
            }
        } else {
            Serial.println("jsonQueue full! Data not sent to parsing from node 2.");
            free(Data);
        }
      }




    }


      vTaskDelay(100 / portTICK_PERIOD_MS);
        
}





void SendToCloud(void *pvParameters) {



      while (true) {
        // Serial.println("In Send to cloud Task");
        // Serial.println("does cloud queue have anythign?");
        // Serial.println(uxQueueMessagesWaiting(cloud_Queue));
          while (uxQueueMessagesWaiting(cloud_Queue) > 0) {  // Check if retry queue has messages
              Serial.println("passed first while");
              char *cloud_data;
              if (xQueueReceive(cloud_Queue, &cloud_data, portMAX_DELAY) == pdTRUE) {
                Serial.println("passed if");
                if (!client.connected()) {
                  Serial.println(" MQTT disconnected. Reconnecting...");
                  connectAWS();  // Reconnect only if disconnected
                }
                StaticJsonDocument<3072> message;
                DeserializationError error = deserializeJson(message, cloud_data);

                if (error) {
                Serial.println("deserealization error from send to cloud task");
                }

                publishMessage(message);
                free(cloud_data);
              vTaskDelay(100 / portTICK_PERIOD_MS);
          } 
      }
  }
}




void ParseTask(void *pvParameters) {
    SemaphoreHandle_t semaphore;
    semaphore = serial2Semaphore;


    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 

        // Serial.print("Free memory ");
        // Serial.println(ESP.getFreeHeap());

        if (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Check if JSON queue has messages
            char *Data;
            Serial.println("Entered parsing");
            if (xQueueReceive(jsonQueue, &Data, portMAX_DELAY) == pdTRUE) {
                int deviceID = DeviceID(Data);

                // Create a copy before sending
                char *DataCopy = strdup(Data);
                if (DataCopy == NULL) {
                    Serial.println("Memory allocation failed for parsetask!");
                    free(DataCopy);
                    free(Data); // Free the original data since it's no longer needed
                    continue;   // Skip this iteration
                }

                if (deviceID == NODE_ID || deviceID==NODE_ID+1) {
                    if (xQueueSend(processing_Queue, &DataCopy, 50 / portTICK_PERIOD_MS) == pdTRUE) {
                        xTaskNotifyGive(Task8Handle); // Process it
                    } else {
                        Serial.println("Processing queue full");
                        free(DataCopy);  // Free if sending fails
                    }
                } else {
                    // if ((cloud_Queue, &DataCopy, 500 / portTICK_PERIOD_MS) == pdTRUE) {
                      if(xQueueSend(cloud_Queue, &DataCopy, 500 / portTICK_PERIOD_MS) == pdTRUE)  {                    
                        Serial.println("Sent data to send down right queue from parse task function");
                        // xTaskNotifyGive(Task2Handle); // Send to node 5
                    } else {
                        Serial.println("Send queue full!");
                        Serial.print("Empty slots in cloud_Queue: ");
                        Serial.println(uxQueueSpacesAvailable(cloud_Queue));
                        free(DataCopy);  // Free if sending fails
                    }
                 
                }
                

                free(Data); // Free the original data after making a copy
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
                StaticJsonDocument<2048> doc;
                DeserializationError error = deserializeJson(doc, Data);
                if (error) {
                    Serial.print("JSON Parse Error: ");
                    Serial.println(error.c_str());
                }

                if (doc.containsKey("statistics")) {
                  Serial.println("Already processed JSON detected. Forwarding directly...");

                  String output;
                  serializeJson(doc, output);

                  // char* jsonCopy = (char*)pvPortMalloc(output.length() + 1);
                  char *jsonCopy = strdup(Data);
                  if (jsonCopy == NULL) {
                    Serial.println("Memory allocation failed for processing task 1!");
                    free(jsonCopy);
                    free(Data); // Free the original data since it's no longer needed
                    continue;   // Skip this iteration
                  }
                  

                  if (xQueueSend(cloud_Queue, &jsonCopy, 50 / portTICK_PERIOD_MS) == pdPASS) {
                      Serial.println("Forwarded already processed JSON to send right node queue");
                      if (uxQueueMessagesWaiting(cloud_Queue) == 1) {
                          xTaskNotifyGive(Task2Handle);
                      }
                  } else {
                      Serial.println("Send queue full! Couldn't forward processed JSON.");
                      free(jsonCopy);
                    }
                

                  free(Data);
                  continue;  // Skip further processing
                }

                JsonArray cells = doc["cells"];
                float totalVoltage = 0, totalCurrent = 0, totalTemperature = 0;
                int totalCells = cells.size();
                int normalCells = 0, compromisedCells = 0;
                std::map<String, int> faultCount;
                JsonArray compromisedArray;

                StaticJsonDocument<1024> summaryDoc;
                summaryDoc["timeInSeconds"] = doc["timeInSeconds"];
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
                
                // Populate fault countsprocesstask
                JsonObject faultCounts = stats.createNestedObject("fault_counts");
                for (auto &entry : faultCount) {
                    faultCounts[entry.first] = entry.second;
                }

                String output;
                serializeJson(summaryDoc, output);


                // Allocate memory for the output JSON string
                  char *jsonCopy = strdup(output.c_str());
                  if (jsonCopy == NULL) {
                    Serial.println("Memory allocation failed for processing task 1!");
                    free(jsonCopy);
                    free(Data); // Free the original data since it's no longer needed
                    continue;   // Skip this iteration
                  }
                  

                  if (xQueueSend(cloud_Queue, &jsonCopy, 500 / portTICK_PERIOD_MS) == pdTRUE) {
                      Serial.println("Forwarded already processed JSON to send right node queue");
                      if (uxQueueMessagesWaiting(cloud_Queue) == 1) {
                          xTaskNotifyGive(Task2Handle);
                      }
                  } else {
                      Serial.println("Send queue full! Couldn't forward processed JSON.");
                      free(jsonCopy);
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
    WriteFromUART(&serial, "Available");
    listen = ReadFromUART(&serial); 
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
        listen = ReadFromUART(serialPort);  
        listen.trim();  
        
        // Serial.println("Listening for PING from " + serial_number + ": [" + listen + "]");  

        if (listen == "PING") {
            Serial.println("Received PING");
            WriteFromUART(serialPort, "Available");

            
            return true;  
        }

        vTaskDelay(1 / portTICK_PERIOD_MS); 
    }

    // Serial.println("Did not receive PING within timeout from " + serial_number);
        
    

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
    doc["module_id"] = "M002";
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
    doc["module_id"] = "M003";
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


String createBatteryJson4() {
    StaticJsonDocument<1024> doc;

    doc["rack_id"] = "R004";
    doc["module_id"] = "M004";
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
    // Serial.println("Sent: PING to " + serial_number);
    while (millis() - startTime < ACK_TIMEOUT) {
        WriteFromUART(serialPort, "PING");
        vTaskDelay(20 / portTICK_PERIOD_MS); 
        String receivedData = ReadFromUART(serialPort);
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


bool peekOldestData(QueueHandle_t queue, char *buffer, size_t bufferSize) {
    if (uxQueueMessagesWaiting(queue) == 0) {
        Serial.println("Queue is empty, nothing to peek.");
        return false;
    }

    // Peek at the oldest message without removing it
    if (xQueuePeek(queue, &buffer, 0) == pdTRUE) {
        Serial.print("Peeked Data: ");
        Serial.println(buffer);
        return true;
    } else {
        Serial.println("Failed to peek data.");
        return false;
    }
}


String ReadFromUART(HardwareSerial *serialPort) {
    String receivedData = "";
    bool messageStarted = false;
    unsigned long startTime = millis();
    const unsigned long timeout = 2000;  // 2 seconds max

    while (millis() - startTime < timeout) {
        if (serialPort->available()) {
            char c = serialPort->read();
            startTime = millis();  // Reset timeout on activity

            if (c == '<') {
                receivedData = "";
                messageStarted = true;
                Serial.println("Received starter UART:");
                continue;
            }

            if (messageStarted) {
                if (c == '>') {
                    Serial.println("Received ender UART:");
                    Serial.println("Returning UART " + receivedData);
                    return receivedData;
                }
                receivedData += c;
            }
        } else {
            delay(2);  // Small wait before checking again
        }
    }

    Serial.println("Timeout waiting for complete message from uart!");
    return "";  // Timeout or no complete message
}



void WriteFromUART(HardwareSerial *serial, const char *input) {
    uint16_t length = strlen(input);
    // Serial.println(String(input));

    const uint16_t maxChunkSize = 64;
    uint16_t numChunks = (length + maxChunkSize - 1) / maxChunkSize;
    // Serial.println("Total length: " + String(length) + ", Number of chunks: " + String(numChunks));

    for (uint16_t chunk = 0; chunk < numChunks; chunk++) {
        uint16_t startIdx = chunk * maxChunkSize;
        uint16_t endIdx = (chunk + 1) * maxChunkSize;
        if (endIdx > length) {
            endIdx = length;
        }

        uint16_t chunkLength = endIdx - startIdx;
        // Serial.println("Chunk " + String(chunk) + " startIdx: " + String(startIdx) + " endIdx: " + String(endIdx) + " chunkLength: " + String(chunkLength));

        if (chunk == 0) {
            serial->write('<');  // Use -> for pointer access
        }

        for (uint16_t i = startIdx; i < endIdx; i++) {
            // delay(2);
            serial->write(input[i]);
            // Serial.println("Sending byte: " + String(input[i]));
        }

        if (chunk == numChunks - 1) {
            serial->write('>');
        }

        delay(10); // Optional delay
    }
}

    String generateRandomID() {
      char idBuffer[20];
      snprintf(idBuffer, sizeof(idBuffer), "B%03d-R%03d-M%03d-C%03d",
              random(1, 2), random(1, 2), random(1, 3), random(1, 3));
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
    client.setKeepAlive(300);

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
    client.subscribe(AWS_IOT_PUBLISH_TOPIC, 1);
}


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

    if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonOutput, false, 1)) {
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
