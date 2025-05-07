
// Iteration Final
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "time.h"

// AWS IoT Topics
// TODO: Fix the topic when the code is functional
#define AWS_IOT_PUBLISH_TOPIC "esp/Module/bulk2"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp/+/data"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;         // Standard Time
const int daylightOffset_sec = 3600;          // Add 3600 during DST
StaticJsonDocument<3072> doc;                 // TOOD: Change the size of the document if Mariam snd a different size JSON object


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
    Serial.println(AWS_IOT_PUBLISH_TOPIC);
    client.subscribe(AWS_IOT_PUBLISH_TOPIC);
}

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

// Main setup function
void setup() {
    Serial.begin(115200);
    connectWiFi();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // To get the epoch time in seconds
    struct tm timeinfo;
      while (!getLocalTime(&timeinfo)) {
          Serial.println("Failed to obtain time");
          delay(1000);
      }
    connectAWS();
    client.onMessage(messageHandler);
}

// Main loop
void loop() {
    client.loop(); // Maintain MQTT connection
    // Mariam-Todo: Replace "generateMessage()" with code for the actual data
    StaticJsonDocument<3072> message = generateMessage();
    publishMessage(message);
    delay(20000);
}