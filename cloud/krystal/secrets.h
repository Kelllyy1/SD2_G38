// TODO: For the code to connect to AWS, this file has to be configured by adding the WiFi credentials, and the correct certificates
// Note: It's hard to share this code because AWS logins require MFA, and the certificates are unique to each account

#include <pgmspace.h>

#define SECRET
#define THINGNAME "esp-connection"

// TODO: Add WiFi credentials and AWS IoT endpoint
// // Home Connection
// const char WIFI_SSID[] = "your-ssid-here";
// const char WIFI_PASSWORD[] = "your-password-here";
// const char AWS_IOT_ENDPOINT[] = "your-endpoint-here";

// // UCF Connection
// const char WIFI_SSID[] = "your-ssid-here";
// const char WIFI_PASSWORD[] = "your-password-here";
// const char AWS_IOT_ENDPOINT[] = "your-endpoint-here";

// Personal Hotspot Connection
// const char WIFI_SSID[] = "your-ssid-here";
// const char WIFI_PASSWORD[] = "your-password-here";
// const char AWS_IOT_ENDPOINT[] = "your-endpoint-here";


// TODO: Add AWS IoT certificates
// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
....CA1...data...here.....
-----END CERTIFICATE-----
)EOF";

// Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
....certificate...data...here.....
-----END CERTIFICATE-----
)KEY";

// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
....private...key...data...here.....
-----END RSA PRIVATE KEY-----
)KEY";

