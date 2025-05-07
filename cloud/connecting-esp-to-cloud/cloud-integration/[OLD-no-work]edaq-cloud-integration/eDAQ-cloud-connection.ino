// Iteration 2

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
    // xTaskCreatePinnedToCore(ParseJsonTask, "Parse", 4096, NULL, 1, &Task3Handle, 0); // Run on Core 0
    xTaskCreatePinnedToCore(SendToNodesTask, "SendData9", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
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


        // // TODO: Mariam, remove this after. Add a 5-second delay after processing all messages -Krystal
        // vTaskDelay(5000 / portTICK_PERIOD_MS);  // 5000 ms = 5 seconds

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
                        publishMessage(message);
                    } else {
                        Serial.println("Did not parse, could not send to cloud");
                        // Handle JSON parsing error (optional logging)
                    }
                    xSemaphoreGive(semaphore);  // Release semaphore
                    vTaskDelay(100 / portTICK_PERIOD_MS); 
                } 
            }
        }
                    // // TODO: Mariam, remove this after. Add a 5-second delay after processing all messages -Krystal
                    // vTaskDelay(5000 / portTICK_PERIOD_MS);  // 5000 ms = 5 seconds
    }
}

// void ParseJsonTask(void *pvParameters) {
//     while (true) {
//       ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//       DynamicJsonDocument doc(1024);


//       while (uxQueueMessagesWaiting(ParseQueue) > 0) {  // Process all messages before sleeping
//           char *Json;
//           if (xQueueReceive(ParseQueue, &Json, portMAX_DELAY) == pdTRUE) {

//               DeserializationError error = deserializeJson(doc, Json);

//               if (error) {
//                   Serial.print("Failed to parse JSON: ");
//                   Serial.println(error.f_str());

//               }
//               // Parse and print the deviceID
//               int deviceID = doc["deviceID"];
//               Serial.print("Device ID: ");
//               Serial.println(deviceID);

//               // Parse and print the voltages for each cell
//               JsonArray cells = doc["cells"].as<JsonArray>();
//               Serial.println("id for each cell:");
//               for (JsonObject cell : cells) {
//                   String id = cell["id"];
//                   Serial.print("id: ");
//                   Serial.println(id);
//               }
//               free(Json); 

//           }


//       vTaskDelay(100 / portTICK_PERIOD_MS);
//       }    
//   }
// }


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

// Krystal - Function to connect ESP32 to Wi-Fi
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
// Function Iteration 1
// // Function to connect ESP32 to AWS IoT
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
//     Serial.println("Subscribed to topic");
// }


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


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Iteration 1
// #include <ArduinoJson.h>
// #include <Wire.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include "secrets.h"
// #include <WiFiClientSecure.h>
// #include <MQTT.h>
// #include <ArduinoJson.h>
// #include "WiFi.h"
// #include "time.h"

// #define NODE_ID 5  // This node's ID
// #define ACK_TIMEOUT 500  
// #define LISTEN_TIMEOUT 500  
// #define WAITFORDATA_TIMEOUT 500  
// #define AWS_IOT_PUBLISH_TOPIC "esp/Module/bulk2"
// #define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

// const char* ntpServer = "pool.ntp.org";
// const long gmtOffset_sec = -5 * 3600;         // Standard Time
// const int daylightOffset_sec = 3600;          // Add 3600 during DST

// // Global Variables
// WiFiClientSecure net;
// MQTTClient client(256);


// SemaphoreHandle_t serial1Semaphore;
// SemaphoreHandle_t serial2Semaphore;



// QueueHandle_t jsonQueue;

// // Task handles for managing FreeRTOS tasks
// TaskHandle_t Task1Handle = NULL;
// TaskHandle_t Task2Handle = NULL;
// TaskHandle_t Task3Handle = NULL;

// bool listenForPing(HardwareSerial *serialPort);
// String WaitForData(HardwareSerial &serial);
// String readFromSerial(HardwareSerial &serialPort);
// bool waitForAck(HardwareSerial *serialPort, SemaphoreHandle_t *semaphore);
// int RouteData(String json);


// int Send_to_Node_9 = 0;
// String Node_9_Data;


// void setup() {
//     Serial.begin(115200);
//     Serial2.begin(9600, SERIAL_8N1, 16, 17);  // communication with node 1
//     Serial1.begin(9600, SERIAL_8N1, 4, 23);  // communication with node 9

//     serial1Semaphore = xSemaphoreCreateBinary();
//     serial2Semaphore = xSemaphoreCreateBinary();
//     xSemaphoreGive(serial1Semaphore);
//     xSemaphoreGive(serial2Semaphore);
    
//     jsonQueue = xQueueCreate(10, sizeof(char *)); 
//     if (jsonQueue == NULL) {
//         Serial.println("jsonQueue creation failed!");
//     } else {
//         Serial.println("jsonQueue created successfully.");
//     }

//     connectWiFi();
//     // TOOD: This may be a problem, because I have to the time each time the data is sent; Consider placing in the Publish function to add to the "TimeInSeconds" attribute to the json object
//     configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//     // To get the epoch time in seconds
//     struct tm timeinfo;
//     while (!getLocalTime(&timeinfo)) {
//         Serial.println("Failed to obtain time");
//         delay(1000);
//     }

//     connectAWS();
//     client.onMessage(messageHandler);
//     // Create FreeRTOS tasks
//     xTaskCreatePinnedToCore(listenForNode1Task, "ListenNode1", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0
//     // xTaskCreatePinnedToCore(ParseJsonTask, "Parse", 4096, NULL, 1, &Task3Handle, 0); // Run on Core 0
//     xTaskCreatePinnedToCore(SendToNodesTask, "SendData9", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
// }

// void loop() {
//     // Empty: Tasks handle execution.
// }



// void listenForNode1Task(void *pvParameters) {

  
//     SemaphoreHandle_t *semaphore;

//     String tempData;
//     int ping;

//     String serial_number;

//     HardwareSerial *serialPort;

//     serialPort = &Serial2;
//     semaphore = &serial2Semaphore;
//     serial_number = "serial2";
//     while (true) {

//     // Serial.println("Reached");
//     Serial.println("listen to node 1 starts");



//     // Check for ping message
  
//     ping = listenForPing(serialPort);
//     if (ping) {
      
//       tempData = WaitForData(*serialPort);

//       Serial.println("Received Data (should be JSON) = " + tempData);

//         if(RouteData(tempData)) {
//           Node_9_Data = tempData;
//         }

//         char *Data = strdup(tempData.c_str());

//         if (Data == NULL) {
//             Serial.println("Memory allocation failed!");
//             continue; // Skip sending if allocation failed
//         }

//         if (xQueueSend(jsonQueue, &Data, portMAX_DELAY) == pdPASS) { // It's killing itself here
//             if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
//                 xTaskNotifyGive(Task2Handle);
//             }
//         } else {
//             Serial.println("Queue full! Data not sent.");
//             free(Data);  // Prevent memory leak if sending fails
//         }
//       }


//         // TODO: Mariam, remove this after. Add a 5-second delay after processing all messages -Krystal
//         vTaskDelay(5000 / portTICK_PERIOD_MS);  // 5000 ms = 5 seconds

//     }


//       vTaskDelay(100 / portTICK_PERIOD_MS);
        
//   }







// void SendToNodesTask(void *pvParameters) {

//     SemaphoreHandle_t semaphore;


//     semaphore = serial2Semaphore;

//     while (true) {
//         Serial.println("reached task");
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing
//         Serial.println("Reached after task");


//         while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages
//             char *DataSend;
//             if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
//                 if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore

//                     StaticJsonDocument<3072> message;
//                     DeserializationError error = deserializeJson(message, DataSend);
//                     if (!error) {
//                         publishMessage(message);
//                     } else {
//                         Serial.println("Did not parse, could not send to cloud");
//                         // Handle JSON parsing error (optional logging)
//                     }
//                     xSemaphoreGive(semaphore);  // Release semaphore
//                     vTaskDelay(100 / portTICK_PERIOD_MS); 
//                 } 
//             }
//         }
//                     // TODO: Mariam, remove this after. Add a 5-second delay after processing all messages -Krystal
//                     vTaskDelay(5000 / portTICK_PERIOD_MS);  // 5000 ms = 5 seconds
//     }
// }

// // void ParseJsonTask(void *pvParameters) {
// //     while (true) {
// //       ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
// //       DynamicJsonDocument doc(1024);


// //       while (uxQueueMessagesWaiting(ParseQueue) > 0) {  // Process all messages before sleeping
// //           char *Json;
// //           if (xQueueReceive(ParseQueue, &Json, portMAX_DELAY) == pdTRUE) {

// //               DeserializationError error = deserializeJson(doc, Json);

// //               if (error) {
// //                   Serial.print("Failed to parse JSON: ");
// //                   Serial.println(error.f_str());

// //               }
// //               // Parse and print the deviceID
// //               int deviceID = doc["deviceID"];
// //               Serial.print("Device ID: ");
// //               Serial.println(deviceID);

// //               // Parse and print the voltages for each cell
// //               JsonArray cells = doc["cells"].as<JsonArray>();
// //               Serial.println("id for each cell:");
// //               for (JsonObject cell : cells) {
// //                   String id = cell["id"];
// //                   Serial.print("id: ");
// //                   Serial.println(id);
// //               }
// //               free(Json); 

// //           }


// //       vTaskDelay(100 / portTICK_PERIOD_MS);
// //       }    
// //   }
// // }


// int RouteData(String json) {


//   int DeviceID;
//   StaticJsonDocument<200> doc;  // Adjust size as needed

//   DeserializationError error = deserializeJson(doc, json);
//   if (error) {
//       Serial.println("JSON parsing failed!");  
//   }


//   DeviceID = doc["deviceID"];
//   if (DeviceID % 4 ==1){
//       Send_to_Node_9 = 1;
//       return 1; 
//   }
//   else {
//     return 0;
//   }
// }

// String WaitForData(HardwareSerial &serial) {

//     String serial_number;
//     String receivedJsonSerial;

//     // Alternate between Serial1 and Serial2
//     if (&serial == &Serial2) {
//         serial_number = "serial2";
//     } else {
//         serial_number = "serial1";
//     }


//     String listen;
//     listen = "";
//     unsigned long startTime = millis();  // Record the start time
//     while (true) {
//     serial.println("Available");
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




// bool waitForAck(HardwareSerial *serialPort, SemaphoreHandle_t *semaphore) {

//     String serial_number;

//     if (serialPort == &Serial1) {
//         serial_number = "serial1";
//     } else if (serialPort == &Serial2) {
//         serial_number = "serial2";
//     } 
    

//     unsigned long startTime = millis();
//     Serial.println("Sent: PING to " + serial_number);
//     while (millis() - startTime < ACK_TIMEOUT) {
//         serialPort->println("PING");
//         delay(50);
//         String receivedData = readFromSerial(*serialPort);
//         if (receivedData == "Available") {
//             Serial.println(serial_number + "responded with available: ");
//             return true;
//         }
//         else if (receivedData == "PING") {
//           xSemaphoreGive(*semaphore);
//           vTaskDelay(2000 / portTICK_PERIOD_MS);
//           xSemaphoreTake(*semaphore, portMAX_DELAY);
          
//         }
//         else {
//           Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
//         }
//     }
//     Serial.println(serial_number + "did not respond in time.");
//     return false;
// }



// String generateRandomID() {
//     char idBuffer[20];
//     snprintf(idBuffer, sizeof(idBuffer), "B%03d-R%03d-M%03d-C%03d",
//              random(1, 1000), random(1, 1000), random(1, 1000), random(1, 1000));
//     return String(idBuffer);
// }

// // Krystal - Function to connect ESP32 to Wi-Fi
// void connectWiFi() {
//     WiFi.mode(WIFI_STA);
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

//     Serial.print("Connecting to Wi-Fi");
//     while (WiFi.status() != WL_CONNECTED) {
//         Serial.print(".");
//         // delay();
//         vTaskDelay(100 / portTICK_PERIOD_MS);
//     }
//     Serial.println("\nWi-Fi Connected!");
// }


// // Function to improve my way of connecting to AWS; so I don't have to reset after each connection
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
//         delay(1000);  // Retry every second

//         if (millis() - startAttemptTime > connectionTimeout) {
//             Serial.println("\nAWS IoT Connection Timeout!");
//             Serial.print("MQTT Error Code: ");
//             Serial.println(client.lastError());
//             return;
//         }
//     }

//     Serial.println("\n Connected to AWS IoT!");

//     // Subscribe to the topic after successful connection
//     Serial.print("Subscribing to topic: ");
//     Serial.println(AWS_IOT_PUBLISH_TOPIC);
//     client.subscribe(AWS_IOT_PUBLISH_TOPIC);
// }
// // Iteration 1
// // // Function to connect ESP32 to AWS IoT
// // void connectAWS() {
// //     // Load AWS Certificates
// //     net.setCACert(AWS_CERT_CA);
// //     net.setCertificate(AWS_CERT_CRT);
// //     net.setPrivateKey(AWS_CERT_PRIVATE);

// //     // Configure MQTT Client
// //     client.begin(AWS_IOT_ENDPOINT, 8883, net);

// //     Serial.print("Connecting to AWS IoT...");
// //     unsigned long startAttemptTime = millis();
// //     const unsigned long connectionTimeout = 10000; // 10 seconds timeout

// //     while (!client.connect(THINGNAME)) {
// //         Serial.print(".");
// //         delay(2000); // Wait 2 seconds before retrying

// //         if (millis() - startAttemptTime > connectionTimeout) {
// //             Serial.println("\nAWS IoT Connection Timeout!");
// //             Serial.print("MQTT Error Code: ");
// //             Serial.println(client.lastError());
// //             return;
// //         }
// //     }

// //     Serial.println("\nConnected to AWS IoT!");

// //     // Subscribe to a topic
// //     Serial.print("Subscribing to topic: ");
// //     Serial.println(AWS_IOT_PUBLISH_TOPIC);
// //     client.subscribe(AWS_IOT_PUBLISH_TOPIC);
// //     Serial.println("Subscribed to topic");
// // }


// // Function to publish data to AWS IoT
// void publishMessage(const JsonDocument& doc) {
//     if (!client.connected()) {
//         Serial.println("Reconnecting to AWS IoT...");
//         if (!client.connect(THINGNAME)) {
//             Serial.println("Reconnection failed!");
//             return;
//         }
//     }

//     char jsonOutput[2048];
//     serializeJson(doc, jsonOutput);

//     Serial.println("Publishing message: ");
//     serializeJsonPretty(doc, Serial);
//     Serial.println();

//     if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonOutput)) {
//         Serial.println("Publish successful!");
//     } else {
//         Serial.print("Publish failed! MQTT Error Code: ");
//         Serial.println(client.lastError());
//     }
// }


// // Function to handle incoming messages from AWS IoT
// void messageHandler(String &topic, String &payload) {
//     Serial.println("Incoming message:");
//     Serial.println("Topic: " + topic);
//     Serial.println("Payload: " + payload);
// }


// // #include <ArduinoJson.h>
// // #include <Wire.h>

// // #include "freertos/FreeRTOS.h"
// // #include "freertos/semphr.h"
// // #include "secrets.h"
// // #include <WiFiClientSecure.h>
// // #include <MQTT.h>
// // #include <ArduinoJson.h>
// // #include "WiFi.h"
// // #include "time.h"

// // #define NODE_ID 5  // This node's ID
// // #define ACK_TIMEOUT 500  
// // #define LISTEN_TIMEOUT 500  
// // #define WAITFORDATA_TIMEOUT 500  
// // #define AWS_IOT_PUBLISH_TOPIC "esp/Module/bulk2"
// // #define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

// // const char* ntpServer = "pool.ntp.org";
// // const long gmtOffset_sec = -5 * 3600;         // Standard Time
// // const int daylightOffset_sec = 3600;          // Add 3600 during DST

// // // Global Variables
// // WiFiClientSecure net;
// // MQTTClient client(256);


// // SemaphoreHandle_t serial1Semaphore;
// // SemaphoreHandle_t serial2Semaphore;



// // QueueHandle_t jsonQueue;

// // // Task handles for managing FreeRTOS tasks
// // TaskHandle_t Task1Handle = NULL;
// // TaskHandle_t Task2Handle = NULL;
// // TaskHandle_t Task3Handle = NULL;

// // bool listenForPing(HardwareSerial *serialPort);
// // String WaitForData(HardwareSerial &serial);
// // String readFromSerial(HardwareSerial &serialPort);
// // bool waitForAck(HardwareSerial *serialPort, SemaphoreHandle_t *semaphore);
// // int RouteData(String json);


// // int Send_to_Node_9 = 0;
// // String Node_9_Data;


// // void setup() {
// //     Serial.begin(115200);
// //     Serial2.begin(9600, SERIAL_8N1, 16, 17);  // communication with node 1
// //     Serial1.begin(9600, SERIAL_8N1, 4, 23);  // communication with node 9

// //     serial1Semaphore = xSemaphoreCreateBinary();
// //     serial2Semaphore = xSemaphoreCreateBinary();
// //     xSemaphoreGive(serial1Semaphore);
// //     xSemaphoreGive(serial2Semaphore);
    
// //     jsonQueue = xQueueCreate(10, sizeof(char *)); 
// //     if (jsonQueue == NULL) {
// //         Serial.println("jsonQueue creation failed!");
// //     } else {
// //         Serial.println("jsonQueue created successfully.");
// //     }

// //     // Connect to the FiFi
// //     connectWiFi();

// //     // TOOD: This may be a problem, because I have to the time each time the data is sent; Consider placing in the Publish function to add to the "TimeInSeconds" attribute to the json object
// //     configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
// //     // To get the epoch time in seconds
// //     struct tm timeinfo;
// //     while (!getLocalTime(&timeinfo)) {
// //         Serial.println("Failed to obtain time");
// //         delay(1000);
// //     }

// //     // Intial connection to AWS IoT
// //     connectAWS();

// //     client.onMessage(messageHandler);
// //     // Create FreeRTOS tasks
// //     xTaskCreatePinnedToCore(listenForNode1Task, "ListenNode1", 4096, NULL, 1, &Task1Handle, 0); // Run on Core 0
// //     // xTaskCreatePinnedToCore(ParseJsonTask, "Parse", 4096, NULL, 1, &Task3Handle, 0); // Run on Core 0
// //     xTaskCreatePinnedToCore(SendToNodesTask, "SendData9", 4096, NULL, 1, &Task2Handle, 1); // Run on Core 1
// // }

// // void loop() {
// //     // Empty: Tasks handle execution.
// // }



// // void listenForNode1Task(void *pvParameters) {

  
// //     SemaphoreHandle_t *semaphore;

// //     String tempData;
// //     int ping;

// //     String serial_number;

// //     HardwareSerial *serialPort;

// //     serialPort = &Serial2;
// //     semaphore = &serial2Semaphore;
// //     serial_number = "serial2";
// //     while (true) {

// //     // Serial.println("Reached");
// //     Serial.println("listen to node 1 starts");



// //     // Check for ping message
  
// //     ping = listenForPing(serialPort);
// //     if (ping) {
      
// //       tempData = WaitForData(*serialPort);

// //       Serial.println("Received Data (should be JSON) = " + tempData);

// //         if(RouteData(tempData)) {
// //           Node_9_Data = tempData;
// //         }

// //         char *Data = strdup(tempData.c_str());

// //         if (Data == NULL) {
// //             Serial.println("Memory allocation failed!");
// //             continue; // Skip sending if allocation failed
// //         }

// //         if (xQueueSend(jsonQueue, &Data, portMAX_DELAY) == pdPASS) { // It's killing itself here
// //             if (uxQueueMessagesWaiting(jsonQueue) == 1) {  // Notify only for first message
// //                 xTaskNotifyGive(Task2Handle);
// //             }
// //         } else {
// //             Serial.println("Queue full! Data not sent.");
// //             free(Data);  // Prevent memory leak if sending fails
// //         }
// //       }




// //     }


// //       vTaskDelay(100 / portTICK_PERIOD_MS);
        
// //   }







// // void SendToNodesTask(void *pvParameters) {

// //     SemaphoreHandle_t semaphore;


// //     semaphore = serial2Semaphore;

// //     while (true) {
// //         Serial.println("reached task");
// //         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for a notification to begin processing
// //         Serial.println("Reached after task");


// //         while (uxQueueMessagesWaiting(jsonQueue) > 0) {  // Process all messages
// //             char *DataSend;
// //             if (xQueueReceive(jsonQueue, &DataSend, portMAX_DELAY) == pdTRUE) {
// //                 if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {  // Acquire semaphore

// //                     StaticJsonDocument<3072> message;
// //                     DeserializationError error = deserializeJson(message, DataSend);
// //                     if (!error) {
// //                         publishMessage(message);
// //                     } else {
// //                         Serial.println("Did not parse, could not send to cloud");
// //                         // Handle JSON parsing error (optional logging)
// //                     }
// //                     xSemaphoreGive(semaphore);  // Release semaphore
// //                     // TODO: Mariam change abck to normal, slowed down system to debug -Krystal
// //                     vTaskDelay(5000 / portTICK_PERIOD_MS); 
// //                 } 
// //             }
// //         }
// //     }
// // }

// // // void ParseJsonTask(void *pvParameters) {
// // //     while (true) {
// // //       ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
// // //       DynamicJsonDocument doc(1024);


// // //       while (uxQueueMessagesWaiting(ParseQueue) > 0) {  // Process all messages before sleeping
// // //           char *Json;
// // //           if (xQueueReceive(ParseQueue, &Json, portMAX_DELAY) == pdTRUE) {

// // //               DeserializationError error = deserializeJson(doc, Json);

// // //               if (error) {
// // //                   Serial.print("Failed to parse JSON: ");
// // //                   Serial.println(error.f_str());

// // //               }
// // //               // Parse and print the deviceID
// // //               int deviceID = doc["deviceID"];
// // //               Serial.print("Device ID: ");
// // //               Serial.println(deviceID);

// // //               // Parse and print the voltages for each cell
// // //               JsonArray cells = doc["cells"].as<JsonArray>();
// // //               Serial.println("id for each cell:");
// // //               for (JsonObject cell : cells) {
// // //                   String id = cell["id"];
// // //                   Serial.print("id: ");
// // //                   Serial.println(id);
// // //               }
// // //               free(Json); 

// // //           }


// // //       vTaskDelay(100 / portTICK_PERIOD_MS);
// // //       }    
// // //   }
// // // }


// // int RouteData(String json) {


// //   int DeviceID;
// //   StaticJsonDocument<200> doc;  // Adjust size as needed

// //   DeserializationError error = deserializeJson(doc, json);
// //   if (error) {
// //       Serial.println("JSON parsing failed!");  
// //   }


// //   DeviceID = doc["deviceID"];
// //   if (DeviceID % 4 ==1){
// //       Send_to_Node_9 = 1;
// //       return 1; 
// //   }
// //   else {
// //     return 0;
// //   }
// // }

// // String WaitForData(HardwareSerial &serial) {

// //     String serial_number;
// //     String receivedJsonSerial;

// //     // Alternate between Serial1 and Serial2
// //     if (&serial == &Serial2) {
// //         serial_number = "serial2";
// //     } else {
// //         serial_number = "serial1";
// //     }


// //     String listen;
// //     listen = "";
// //     unsigned long startTime = millis();  // Record the start time
// //     while (true) {
// //     serial.println("Available");
// //     listen = readFromSerial(serial); 
// //     if (listen != "PING" && listen != "") {
// //         Serial.println("Did not receive PING, instead received: " + listen + "from " + serial_number);
// //         receivedJsonSerial = listen;
// //         return receivedJsonSerial;
// //     }

// //     // Print status every time a PING is still being received
// //     Serial.println("receiving" + listen + " PING after Available was sent to " + serial_number);

// //     // Check if the 500 ms timeout has passed
// //     if (millis() - startTime >= WAITFORDATA_TIMEOUT) {
// //         Serial.println("Timeout reached, stopping PING loop.");
// //         return "no data";
// //     }

      
// //   }


    
// // }

// // bool listenForPing(HardwareSerial *serialPort) {
// //     String listen = "";
// //     unsigned long startTime = millis();  

// //     String serial_number;

// //     if (serialPort == &Serial1) {
// //         serial_number = "serial1";
// //     } else if (serialPort == &Serial2) {
// //         serial_number = "serial2";
// //     } 

    
    
// //     while (millis() - startTime < LISTEN_TIMEOUT) {  
// //         listen = readFromSerial(*serialPort);  
// //         listen.trim();  
        
// //         Serial.println("Listening for PING from " + serial_number + ": [" + listen + "]");  

// //         if (listen == "PING") {
// //             Serial.println("Received PING");
// //             serialPort->println("Available");  

            
// //             return true;  
// //         }

// //         vTaskDelay(1 / portTICK_PERIOD_MS); 
// //     }

// //     Serial.println("Did not receive PING within timeout from " + serial_number);
        
    

// //     return false;
// // }








// // String readFromSerial(HardwareSerial &serial) {


// //     String receivedData = "";

// //     // Take the semaphore before accessing the serial line
    
// //     while (serial.available()) {
// //         char c = serial.read();

// //         if (c == '\n' || c == '\r') {
// //             break;  // Stop reading at newline or carriage return
// //         }

// //         receivedData += c;
// //         delay(2);  // Use delay to prevent task starvation
// //     }
// //     receivedData.trim();  // Remove unwanted spaces or newline chars


    

// //     return receivedData;
// // }




// // bool waitForAck(HardwareSerial *serialPort, SemaphoreHandle_t *semaphore) {

// //     String serial_number;

// //     if (serialPort == &Serial1) {
// //         serial_number = "serial1";
// //     } else if (serialPort == &Serial2) {
// //         serial_number = "serial2";
// //     } 
    

// //     unsigned long startTime = millis();
// //     Serial.println("Sent: PING to " + serial_number);
// //     while (millis() - startTime < ACK_TIMEOUT) {
// //         serialPort->println("PING");
// //         delay(50);
// //         String receivedData = readFromSerial(*serialPort);
// //         if (receivedData == "Available") {
// //             Serial.println(serial_number + "responded with available: ");
// //             return true;
// //         }
// //         else if (receivedData == "PING") {
// //           xSemaphoreGive(*semaphore);
// //           vTaskDelay(2000 / portTICK_PERIOD_MS);
// //           xSemaphoreTake(*semaphore, portMAX_DELAY);
          
// //         }
// //         else {
// //           Serial.println("Received Data after sending PING (waiitng for avaiable) from : " + serial_number  + receivedData);
// //         }
// //     }
// //     Serial.println(serial_number + "did not respond in time.");
// //     return false;
// // }



// // String generateRandomID() {
// //     char idBuffer[20];
// //     snprintf(idBuffer, sizeof(idBuffer), "B%03d-R%03d-M%03d-C%03d",
// //              random(1, 1000), random(1, 1000), random(1, 1000), random(1, 1000));
// //     return String(idBuffer);
// // }

// // // Krystal - Function to connect ESP32 to Wi-Fi
// // void connectWiFi() {
// //     WiFi.mode(WIFI_STA);
// //     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

// //     Serial.print("Connecting to Wi-Fi");
// //     while (WiFi.status() != WL_CONNECTED) {
// //         Serial.print(".");
// //         // delay();
// //         vTaskDelay(100 / portTICK_PERIOD_MS);
// //     }
// //     Serial.println("\nWi-Fi Connected!");
// // }

// // // Improved function to conenct to the cloud
// // void connectAWS() {
// //     // Load AWS Certificates
// //     net.setCACert(AWS_CERT_CA);
// //     net.setCertificate(AWS_CERT_CRT);
// //     net.setPrivateKey(AWS_CERT_PRIVATE);

// //     // Configure MQTT Client
// //     client.begin(AWS_IOT_ENDPOINT, 8883, net);

// //     Serial.print("Connecting to AWS IoT...");
// //     unsigned long startAttemptTime = millis();
// //     const unsigned long connectionTimeout = 10000; // 10 seconds timeout

// //     while (!client.connect(THINGNAME)) {
// //         Serial.print(".");
// //         delay(1000);  // Retry every second

// //         if (millis() - startAttemptTime > connectionTimeout) {
// //             Serial.println("\nAWS IoT Connection Timeout!");
// //             Serial.print("MQTT Error Code: ");
// //             Serial.println(client.lastError());
// //             return;
// //         }
// //     }

// //     Serial.println("\n Connected to AWS IoT!");

// //     // Subscribe to the topic after successful connection
// //     Serial.print("Subscribing to topic: ");
// //     Serial.println(AWS_IOT_PUBLISH_TOPIC);
// //     client.subscribe(AWS_IOT_PUBLISH_TOPIC);
// // }

// // // Iteration 1
// // // Function to connect ESP32 to AWS IoT
// // // void connectAWS() {
// // //     // Load AWS Certificates
// // //     net.setCACert(AWS_CERT_CA);
// // //     net.setCertificate(AWS_CERT_CRT);
// // //     net.setPrivateKey(AWS_CERT_PRIVATE);

// // //     // Configure MQTT Client
// // //     client.begin(AWS_IOT_ENDPOINT, 8883, net);

// // //     Serial.print("Connecting to AWS IoT...");
// // //     unsigned long startAttemptTime = millis();
// // //     const unsigned long connectionTimeout = 10000; // 10 seconds timeout

// // //     while (!client.connect(THINGNAME)) {
// // //         Serial.print(".");
// // //         delay(2000); // Wait 2 seconds before retrying

// // //         if (millis() - startAttemptTime > connectionTimeout) {
// // //             Serial.println("\nAWS IoT Connection Timeout!");
// // //             Serial.print("MQTT Error Code: ");
// // //             Serial.println(client.lastError());
// // //             return;
// // //         }
// // //     }

// // //     Serial.println("\nConnected to AWS IoT!");

// // //     // Subscribe to a topic
// // //     Serial.print("Subscribing to topic: ");
// // //     Serial.println(AWS_IOT_PUBLISH_TOPIC);
// // //     client.subscribe(AWS_IOT_PUBLISH_TOPIC);
// // //     Serial.println("Subscribed to topic");
// // // }

// // // Function to randomly generate the JSON packet that will be published to a topic in the cloud
// // // returns the JSON object.
// // // TODO: Fix what the device sends
// // StaticJsonDocument<3072> generateMessage() {
// //     StaticJsonDocument<3072> doc;

// //     // Identifiers
// //     doc["timeInSeconds"] = String(time(nullptr));
// //     doc["moduleID"] = "M009";
// //     doc["deviceID"] = random(1, 11);

// //     // Statistics + Compromised array
// //     JsonObject stats = doc.createNestedObject("statistics");
// //     JsonArray compromised = doc.createNestedArray("compromised_cells");

// //     // STEP 1: Total cells and guaranteed ≥ 1 compromised
// //     int total_cells = random(1, 11); // 1–10
// //     int max_normal = max(0, total_cells - 1); // Leave space for 1 compromised
// //     int normal_cells = constrain(random(4, 8), 0, max_normal);
// //     int compromised_cells = total_cells - normal_cells;

// //     // STEP 2: Fault distribution (must add to total_cells)
// //     int fault_normal = 0, fault_oc = 0, fault_oh = 0, fault_od = 0;
// //     int remaining = total_cells;

// //     fault_normal = min((int)random(4, 8), remaining);
// //     remaining -= fault_normal;
// //     if (remaining > 0) { fault_oc = random(0, remaining + 1); remaining -= fault_oc; }
// //     if (remaining > 0) { fault_oh = random(0, remaining + 1); remaining -= fault_oh; }
// //     if (remaining > 0) { fault_od = remaining; }

// //     // STEP 3: Generate compromised cell values
// //     float sum_voltage = 0.0, sum_current = 0.0, sum_temp = 0.0;

// //     for (int i = 0; i < compromised_cells; i++) {
// //         float voltage = 2.7;
// //         float current = random(200, 500) / 100.0;
// //         float temp = random(2300, 2700) / 100.0;

// //         sum_voltage += voltage;
// //         sum_current += current;
// //         sum_temp += temp;

// //         String fault;
// //         if (fault_oc > 0) { fault = "Over_current"; fault_oc--; }
// //         else if (fault_oh > 0) { fault = "Overheating"; fault_oh--; }
// //         else if (fault_od > 0) { fault = "Over_discharge"; fault_od--; }
// //         else { fault = "Normal"; fault_normal--; }

// //         JsonObject cell = compromised.createNestedObject();
// //         cell["id"] = generateRandomID();
// //         cell["voltage"] = voltage;
// //         cell["curr"] = current;
// //         cell["temperature"] = temp;
// //         cell["status"] = "Compromised";
// //         cell["faults"] = fault;
// //     }

// //     // STEP 4: Stats block
// //     stats["total_cells"] = total_cells;
// //     stats["normal_cells"] = normal_cells;
// //     stats["compromised_cells"] = compromised_cells;
// //     stats["average_voltage"] = compromised_cells > 0 ? sum_voltage / compromised_cells : 0.0;
// //     stats["average_current"] = compromised_cells > 0 ? sum_current / compromised_cells : 0.0;
// //     stats["average_temperature"] = compromised_cells > 0 ? sum_temp / compromised_cells : 0.0;

// //     JsonObject fault_counts = stats.createNestedObject("fault_counts");
// //     fault_counts["Normal"] = fault_normal;
// //     fault_counts["Over_current"] = fault_oc;
// //     fault_counts["Overheating"] = fault_oh;
// //     fault_counts["Over_discharge"] = fault_od;

// //     return doc;
// // }



// // // Function to publish data to AWS IoT
// // void publishMessage(const JsonDocument& doc) {
// //     if (!client.connected()) {
// //         Serial.println("Reconnecting to AWS IoT...");
// //         if (!client.connect(THINGNAME)) {
// //             Serial.println("Reconnection failed!");
// //             return;
// //         }
// //     }

// //     char jsonOutput[2048];
// //     serializeJson(doc, jsonOutput);

// //     Serial.println("Publishing message: ");
// //     serializeJsonPretty(doc, Serial);
// //     Serial.println();

// //     if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonOutput)) {
// //         Serial.println("Publish successful!");
// //     } else {
// //         Serial.print("Publish failed! MQTT Error Code: ");
// //         Serial.println(client.lastError());
// //     }
// // }


// // // Function to handle incoming messages from AWS IoT
// // void messageHandler(String &topic, String &payload) {
// //     Serial.println("Incoming message:");
// //     Serial.println("Topic: " + topic);
// //     Serial.println("Payload: " + payload);
// // }
