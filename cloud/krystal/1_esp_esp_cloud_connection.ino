// TODO: Implement code to retrieve the current epoch time in seconds from somewhere;
    // IoT SiteWise rejects timestamps incorrectly formatted, older than 7 days, or beyond 10 minutes in the future

// Iteration 2
#include "secrets.h"  // Include your AWS IoT credentials
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include "WiFi.h"

// AWS IoT Topics (Ensure these exist in AWS IoT Core)
#define AWS_IOT_PUBLISH_TOPIC "esp/Module1/data"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

// Global Variables
WiFiClientSecure net;
MQTTClient client(256);

// Function to generate a random device ID in the format "Bxxx-Rxxx-Mxxx-Cxxx"
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

// Function to connect ESP32 to AWS IoT
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
        delay(2000); // Wait 2 seconds before retrying

        if (millis() - startAttemptTime > connectionTimeout) {
            Serial.println("\nAWS IoT Connection Timeout!");
            Serial.print("MQTT Error Code: ");
            Serial.println(client.lastError());
            return;
        }
    }

    Serial.println("\nConnected to AWS IoT!");

    // Subscribe to a topic
    Serial.print("Subscribing to topic: ");
    Serial.println(AWS_IOT_SUBSCRIBE_TOPIC);
    client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
}

// Function to publish data to AWS IoT
void publishMessage() {
    if (!client.connected()) {
        Serial.println("Reconnecting to AWS IoT...");
        if (!client.connect(THINGNAME)) {
            Serial.println("Reconnection failed!");
            return;
        }
    }

    // Generate a random device ID
    String deviceID = generateRandomID();

    // Create JSON payload
    StaticJsonDocument<256> doc;

    // TODO: Fill in the timestamp here in the "timeInSeconds" field once it is implemnted, to ensure the data is not rejected by IoT SiteWise
    doc["timeInSeconds"] = 1742351662;
    doc["id"] = deviceID;
    doc["temperature"] = random(2500, 3000) / 100.0;
    doc["voltage"] = random(250, 300) / 100.0;
    doc["curr"] = random(100, 1000) / 100.0;
    doc["status"] = "Normal";
    doc["faults"] = "Normal";

    char jsonOutput[256];
    serializeJson(doc, jsonOutput);

    Serial.println("Publishing message: ");
    Serial.println(jsonOutput);

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

// Main setup function
void setup() {
    Serial.begin(115200);
    connectWiFi();
    connectAWS();
    client.onMessage(messageHandler);
}

// Main loop
void loop() {
    client.loop(); // Maintain MQTT connection
    publishMessage();
    delay(20000);
}


// [DEPRECATED] Iteration 1
// #include "secrets.h"
// #include <WiFiClientSecure.h>
// #include <MQTT.h>
// #include <ArduinoJson.h>
// #include "WiFi.h"
// #include <HTTPClient.h>

// // The MQTT Topic that this device should publish/subsribe to
// #define AWS_IOT_PUBLISH_TOPIC "esp/+/data"
// #define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

// WiFiClientSecure net = WiFiClientSecure();
// MQTTClient client = MQTTClient(256);
// // String dateTime;

// // Function to connect to AWS IoT Console
// void connectAWS()
// {
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

//   Serial.println("Connecting to Wi-Fi");

//   while (WiFi.status() != WL_CONNECTED){
//     delay(1000);
//     Serial.print(".");
//   }

//   if (WiFi.status() == WL_CONNECTED){
//     Serial.println("Wi-Fi Connected!");
//   } else {
//     Serial.println("Wi-Fi Connection Failed!");
//   }

//   // configure WiFiClientSecure to use the AWS IoT device credentials
//   net.setCACert(AWS_CERT_CA);
//   net.setCertificate(AWS_CERT_CRT);
//   net.setPrivateKey(AWS_CERT_PRIVATE);

//   // Connect to the MQTT Broker on the AWS endpoint we defined earlier
//   client.begin(AWS_IOT_ENDPOINT, 8883, net);

//   Serial.print("Connecting to AWS IoT");

//   while(!client.connect(THINGNAME)) {
//     Serial.print(".");
//     delay(100);
//   }

//   if(!client.connected()){
//     // Serial.println("AWS IoT Timeout!");
//     Serial.println("AWS IoT T!");
//     return;
//   }

//   // Subscribe to a topic
//   client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

//   Serial.println("AWS IoT Connected!");
// }

// // Function to generate a random device ID in the format "Bxxx-Rxxx-Mxxx-Cxxx"
// String generateRandomID() {
//     char idBuffer[20];
//     snprintf(idBuffer, sizeof(idBuffer), "B%03d-R%03d-M%03d-C%03d",
//              random(1, 1000), random(1, 1000), random(1, 1000), random(1, 1000));
//     return String(idBuffer);
// }

// // Function to publish a message
// void publishMessage()
// {
//   // Test data to be sent to the cloud

//     // Get timestamp
//     char timestamp[25];
//     // snprintf(timestamp, sizeof(timestamp), "2025-02-13T%02d:%02d:%02d", random(0, 24), random(0, 60), random(0, 60));


//     float voltage = random(250, 300) / 100.0;  // Voltage between 2.5V - 3.0V
//     float curr = random(200, 300) / 100.0;  // Current between 2.0A - 3.0A
//     float temperature = random(2500, 3000) / 100.0;  // Temperature between 25°C - 30°C
//     // unsigned long timestamp = getCurrentTime();
//     String deviceID = generateRandomID();  // Generate a random ID
//     // Test Object 2
//     StaticJsonDocument<1024> doc;

//     doc["timeInSeconds"] = 1742337415;
//     doc["id"] = deviceID;
//     doc["voltage"] = voltage;
//     doc["curr"] = curr;
//     doc["temperature"] = temperature;
//     doc["status"] = "Normal";
//     doc["faults"] = "Normal";

//     char jsonOutput[1024];
//     serializeJson(doc, jsonOutput);


//   // Publish to AWS IoT
//   Serial.println("Publishing message: ");
//     // Print JSON output to Serial Monitor
//     Serial.println(jsonOutput);
//     Serial.println(strlen(jsonOutput)); // Print the size of the message. AWS IoT max size is 12KB

// if (!client.connected()) {
//     // Serial.println("MQTT client disconnected. Reconnecting...");
//     Serial.println("Reconnecting...");
//     client.connect(THINGNAME);  // Reattempt connection
// }
//   if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonOutput)) {
//     Serial.println("Publish successful!");
// } else {
//     // Serial.print("Publish failed, MQTT Error Code: ");
//     Serial.print("Published. ");
//     Serial.println(client.lastError());  // Get the last error code
// }

// }

// // IDK what this does exactly???
// void messageHandler(String &topic, String &payload) {
//   Serial.println("incoming: " + topic + " - " + payload);
// }


// void setup() {
//   // put your setup code here, to run once:
//   Serial.begin(115200);
//   connectAWS();
// }

// void loop() {
//   // put your main code here, to run repeatedly:
//   client.loop();  // Maintain the MQTT connection
//   publishMessage();
//   delay(20000);

// }
