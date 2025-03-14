#include <SPI.h>
#include <Wire.h>
#include <SC16IS752.h>

SC16IS752 i2cuart = SC16IS752(SC16IS750_PROTOCOL_I2C,0x90);
//SC16IS750_ADDRESS_AA

//Connect TX(A-CH) and RX(B-CH) with a short wire
//Connect TX(B-CH) and RX(A-CH) with a short wire




void setup() 
{
    Serial.begin(115200);
    Serial.println("Start testing");
    // UART to Serial Bridge Initialization
    i2cuart.begin(SC16IS752_DEFAULT_SPEED, SC16IS752_DEFAULT_SPEED); //baudrate setting
    if (i2cuart.ping()!=1) {
        Serial.println("Device not found");
        while(1);
    } else {
        Serial.println("Device found");
    }

}






void loop() 
{

    delay(10);
    if (i2cuart.available(SC16IS752_CHANNEL_A)!=0) {


      char c = i2cuart.read(SC16IS752_CHANNEL_A);
      Serial.print(c);

    }
    else {
        Serial.println("nothing");

    }

    delay(400);


}