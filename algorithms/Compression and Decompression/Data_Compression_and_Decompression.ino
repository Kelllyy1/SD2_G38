#include <Arduino.h>
#include <ArduinoJson.h>
#include "mbedtls/base64.h"

const char* status_dict[] = {"Normal", "Compromised"};
const char* faults_dict[] = {"Normal", "Over_current", "Overheating", "Over_discharge"};

// Apply dictionary encoding and field renaming
void compressCells(JsonDocument& doc) {
    JsonArray cells = doc["cells"].as<JsonArray>();
    JsonArray compact = doc.createNestedArray("c");

    for (JsonObject cell : cells) {
        JsonArray item = compact.createNestedArray();
        item.add(cell["v"] = round(cell["voltage"].as<float>() * 10) / 10.0);
        item.add(cell["c"] = round(cell["current"].as<float>() * 100) / 100.0);
        item.add(cell["t"] = (int)round(cell["temperature"].as<float>()));

        int status = 0;
        for (int i = 0; i < 2; i++) if (strcmp(cell["status"], status_dict[i]) == 0) status = i;
        int faults = 0;
        for (int i = 0; i < 4; i++) if (strcmp(cell["faults"], faults_dict[i]) == 0) faults = i;

        item.add(status);
        item.add(faults);
    }

    doc.remove("cells");
    doc.remove("node_id");
    doc.remove("rack_id");
    doc.remove("module_id");
    doc.remove("deviceID");
}

void decompressCells(JsonDocument& doc) {
    JsonArray compact = doc["c"].as<JsonArray>();
    JsonArray cells = doc.createNestedArray("cells");

    int i = 1;
    for (JsonArray entry : compact) {
        JsonObject cell = cells.createNestedObject();
        cell["id"] = "C" + String(i++);
        cell["voltage"] = entry[0];
        cell["current"] = entry[1];
        cell["temperature"] = entry[2];
        cell["status"] = status_dict[entry[3].as<int>()];
        cell["faults"] = faults_dict[entry[4].as<int>()];
    }
    doc.remove("c");
}

String applyBase64Encoding(String input) {
    size_t output_len = 0;
    size_t input_len = input.length();
    const char* input_data = input.c_str();
    unsigned char output[2048] = {0};

    int ret = mbedtls_base64_encode(output, sizeof(output), &output_len,
                                    (const unsigned char*)input_data, input_len);
    if (ret != 0) {
        return "ENCODE_ERR";
    }
    return String((char*)output);
}

String applyBase64Decoding(String input) {
    size_t output_len = 0;
    size_t input_len = input.length();
    unsigned char output[2048] = {0};

    int ret = mbedtls_base64_decode(output, sizeof(output), &output_len,
                                    (const unsigned char*)input.c_str(), input_len);
    if (ret != 0) {
        return "DECODE_ERR";
    }
    return String((char*)output);
}

String compressJson(JsonDocument& doc) {
    compressCells(doc);
    String minifiedJson;
    serializeJson(doc, minifiedJson);
    return applyBase64Encoding(minifiedJson);
}

String decompressJson(String compressedData) {
    String decoded = applyBase64Decoding(compressedData);
    if (decoded == "DECODE_ERR") return "BASE64 ERROR";

    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, decoded);
    if (err) return "JSON_PARSE_ERR";

    decompressCells(doc);
    String prettyJson;
    serializeJson(doc, prettyJson);
    return prettyJson;
}

String createSimData(int node_id) {
    StaticJsonDocument<2048> doc;
    doc["node_id"] = node_id;
    doc["rack_id"] = "R001";
    doc["module_id"] = "M001";
    doc["deviceID"] = 1;
    JsonArray cells = doc.createNestedArray("cells");

    for (int i = 1; i <= 10; i++) {
        JsonObject cell = cells.createNestedObject();
        cell["id"] = "B001-R001-M001-C00" + String(i);
        cell["voltage"] = 2.7;
        cell["current"] = random(2500, 5000) / 1000.0;
        cell["temperature"] = random(23000, 27000) / 1000.0;
        cell["status"] = status_dict[random(0, 2)];
        cell["faults"] = faults_dict[random(0, 4)];
    }

    serializeJsonPretty(doc, Serial);
    Serial.println();

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    String jsonString = createSimData(1);
    Serial.println("Original JSON:");
    Serial.println(jsonString);
    Serial.println("Original Size: " + String(jsonString.length()) + " bytes\n");

    StaticJsonDocument<2048> doc;
    deserializeJson(doc, jsonString);

    String compressed = compressJson(doc);
    Serial.println("Compressed:");
    Serial.println(compressed);
    Serial.println("Compressed Size: " + String(compressed.length()) + " bytes\n");

    String decompressed = decompressJson(compressed);
    Serial.println("Decompressed:");
    Serial.println(decompressed);
    Serial.println("Decompressed Size: " + String(decompressed.length()) + " bytes");
}

void loop() {}
