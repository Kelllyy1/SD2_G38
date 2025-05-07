#include <pgmspace.h>

#define SECRET
#define THINGNAME "ESP32_Module_01"

// Personal Hotspot - my hotspot, you have to replace WIFI_SSID with your wifi name and WIFI_PASSWORD with your password
const char WIFI_SSID[] = "Me";
const char WIFI_PASSWORD[] = "EdUcAtIoN.";
const char AWS_IOT_ENDPOINT[] = "a3s15ecxo7nqh3-ats.iot.us-east-2.amazonaws.com";

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

// Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----

)KEY";

// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----

)KEY";
