#include "AsebaLite/AsebaLite.h"
//#include "Monitor/Monitor.h"

int led = D7; // built-in LED
int phi = D0;

//Monitor monitor;

AsebaLite::AsebaBuffer asebaBuffer;
unsigned long tick = millis();

void setup() {
    pinMode(led, OUTPUT);     // initialize D7 pin as digital output
    pinMode(phi, OUTPUT);    // initialize D0 pin as digital output
    digitalWrite(led, LOW);
    digitalWrite(phi, LOW);

    Serial.begin(9600);       // usb monitor
    Serial1.begin(9600);      // tx/rx pins
    //    monitor.ledOff();
    //    monitor.begin();
    //    monitor.variable("beacon_tick",&tick, INT);

    asebaBuffer.resetBuffer();

    asebaBuffer.sendETX();
    asebaBuffer.sendACK();
}


void loop() {
    //    monitor.report();
    digitalWrite(led, LOW);
    //Serial.print(".");

    if (Serial1.available() > 0) {
        uint8 incomingByte = (uint8)Serial1.read();
        asebaBuffer.appendBuffer(&incomingByte, 1);
        //Serial.print(incomingByte, HEX);
        tick = millis();
        //Serial.println(" tick = " + String(tick));
    }
    else if (millis() - tick > 10000) {
        Serial.println(" millis() = " + String(millis()) + " tick = " + String(tick));
        asebaBuffer.sendACK();
        //delay(5000);
        //asebaBuffer.sendACK();
        tick = millis();
    }

    uint8 data[500];
    uint16 m_length;
    uint16 m_source;
    uint16 m_type;
    if (asebaBuffer.messageReady()) {
        Serial.println("; message ready:");
        uint16 len = asebaBuffer.getBuffer(data, 6);
        memcpy(&m_length, data,     2);
        memcpy(&m_source, data + 2, 2);
        memcpy(&m_type,   data + 4, 2);
        Serial.println("Message read " + String(len) + ": length " + String(m_length) + " source " + String(m_source) + " type " + String(m_type,HEX));
        asebaBuffer.parseBuffer();
        //uint16 length = asebaBuffer.getBufferPayload(data, 500);
        //Serial.println();
        //Serial.println("Payload length " + String(length));
        asebaBuffer.resetBuffer();
        asebaBuffer.sendACK();
    }

    digitalWrite(led, HIGH);
}
