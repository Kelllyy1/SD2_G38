// Data Compression and Decompression for Statistics.
#include <Arduino.h>
#include <ArduinoJson.h>
#include "mbedtls/base64.h"

const char* faults_dict[] = {"Normal", "Over_current", "Overheating", "Over_discharge"};

void compressStatistics(JsonDocument& doc) {
  // Rename statistics block
  JsonObject stats = doc["statistics"];
  JsonObject compact = doc.createNestedObject("s");
  compact["t"] = stats["total_cells"];
  compact["n"] = stats["normal_cells"];
  compact["c"] = stats["compromised_cells"];
  compact["i"] = round(stats["average_current"].as<float>() * 100) / 100.0;
  compact["v"] = round(stats["average_voltage"].as<float>() * 100) / 100.0;
  compact["x"] = round(stats["average_temperature"].as<float>());

  JsonObject fault_counts = stats["fault_counts"];
  JsonArray f = compact.createNestedArray("f");
  f.add(fault_counts["Normal"]);
  f.add(fault_counts["Over_current"]);
  f.add(fault_counts["Overheating"]);
  f.add(fault_counts["Over_discharge"]);

  doc.remove("statistics");

  // Compress compromised_cells
  JsonArray input = doc["compromised_cells"].as<JsonArray>();
  JsonArray compactCells = doc.createNestedArray("d");
  for (JsonObject cell : input) {
    JsonArray item = compactCells.createNestedArray();
    item.add(cell["v"] = round(cell["voltage"].as<float>() * 10) / 10.0);
    item.add(cell["c"] = round(cell["current"].as<float>() * 100) / 100.0);
    item.add(cell["t"] = round(cell["temperature"].as<float>()));
    int faults = 0;
    for (int i = 0; i < 4; i++) if (strcmp(cell["faults"], faults_dict[i]) == 0) faults = i;
    item.add(faults);
  }
  doc.remove("compromised_cells");
}

void decompressStatistics(JsonDocument& doc) {
  JsonObject compact = doc["s"];
  JsonObject stats = doc.createNestedObject("statistics");
  stats["total_cells"] = compact["t"];
  stats["normal_cells"] = compact["n"];
  stats["compromised_cells"] = compact["c"];
  stats["average_current"] = compact["i"];
  stats["average_voltage"] = compact["v"];
  stats["average_temperature"] = compact["x"];

  JsonArray f = compact["f"].as<JsonArray>();
  JsonObject fc = stats.createNestedObject("fault_counts");
  for (int i = 0; i < 4; i++) fc[faults_dict[i]] = f[i];

  doc.remove("s");

  JsonArray compactCells = doc["d"].as<JsonArray>();
  JsonArray cells = doc.createNestedArray("compromised_cells");
  for (int i = 0; i < compactCells.size(); i++) {
    JsonArray entry = compactCells[i];
    JsonObject cell = cells.createNestedObject();
    cell["id"] = "C" + String(i + 1);
    cell["voltage"] = entry[0];
    cell["current"] = entry[1];
    cell["temperature"] = entry[2];
    cell["status"] = "Compromised";
    cell["faults"] = faults_dict[entry[3].as<int>()];
  }
  doc.remove("d");
}

String compressJson(JsonDocument& doc) {
  compressStatistics(doc);
  String compact;
  serializeJson(doc, compact);
  size_t outlen;
  unsigned char out[1024] = {0};
  mbedtls_base64_encode(out, sizeof(out), &outlen, (const unsigned char*)compact.c_str(), compact.length());
  return String((char*)out);
}

String decompressJson(String base64) {
  size_t outlen;
  unsigned char out[1024] = {0};
  mbedtls_base64_decode(out, sizeof(out), &outlen, (const unsigned char*)base64.c_str(), base64.length());
  StaticJsonDocument<1024> doc;
  deserializeJson(doc, (char*)out);
  decompressStatistics(doc);
  String restored;
  serializeJson(doc, restored);
  return restored;
}

void setup() {
  Serial.begin(115200);
  StaticJsonDocument<1024> doc;
  deserializeJson(doc, F("{\"statistics\":{\"total_cells\":9,\"normal_cells\":5,\"compromised_cells\":4,\"average_current\":4.0753,\"average_voltage\":4.0753,\"average_temperature\":25.215,\"fault_counts\":{\"Normal\":5,\"Over_current\":2,\"Overheating\":1,\"Over_discharge\":1}},\"compromised_cells\":[{\"id\":\"B001-R001-M001-C002\",\"voltage\":2.7,\"current\":3.563,\"temperature\":25.369,\"status\":\"Compromised\",\"faults\":\"Over_current\"},{\"id\":\"B001-R001-M001-C004\",\"voltage\":2.7,\"current\":2.503,\"temperature\":25.546,\"status\":\"Compromised\",\"faults\":\"Overheating\"},{\"id\":\"B001-R001-M001-C005\",\"voltage\":2.7,\"current\":4.396,\"temperature\":25.392,\"status\":\"Compromised\",\"faults\":\"Over_discharge\"},{\"id\":\"B001-R001-M001-C008\",\"voltage\":2.7,\"current\":4.711,\"temperature\":25.594,\"status\":\"Compromised\",\"faults\":\"Over_current\"}]}"));

  String original;
  serializeJson(doc, original);
  Serial.println("Original:");
  Serial.println(original);
  Serial.println("Size: " + String(original.length()));

  String compressed = compressJson(doc);
  Serial.println("\nCompressed:");
  Serial.println(compressed);
  Serial.println("Size: " + String(compressed.length()));

  String decompressed = decompressJson(compressed);
  Serial.println("\nDecompressed:");
  Serial.println(decompressed);
  Serial.println("Size: " + String(decompressed.length()));
}

void loop() {}
