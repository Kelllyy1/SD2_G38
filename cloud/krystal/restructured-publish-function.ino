// New publish function to accept a string instead of a JSON object

void publishMessage(const String& messagePayload) {
    if (!client.connected()) {
        Serial.println("Reconnecting to AWS IoT...");
        if (!client.connect(THINGNAME)) {
            Serial.println("Reconnection failed!");
            return;
        }
    }

    Serial.println("Publishing message: ");
    Serial.println(messagePayload);

    if (client.publish(AWS_IOT_PUBLISH_TOPIC, messagePayload.c_str())) {
        Serial.println("Publish successful!");
    } else {
        Serial.print("Publish failed! MQTT Error Code: ");
        Serial.println(client.lastError());
    }
}
