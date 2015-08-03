#include "AsebaLite/AsebaLite.h"
#include "Monitor/Monitor.h"

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
    
    delay(500);
    Serial.print("sending ETX...");
    Serial1.write(ETX);
    Serial1.flush();
    
    uint8 incomingByte;
    int avail = Serial1.available();
    if (avail > 0) {
        while (avail--) {
            incomingByte = (uint8)Serial1.read();
        }
        Serial.print(incomingByte, HEX);
    }

    delay(500);
    Serial.println("done.");
    Serial.println("now available " + String(Serial1.available()));
    Serial1.write(ACK);
    Serial1.flush();
}


void loop() {
    //    monitor.report();
    digitalWrite(led, LOW);
    //Serial.print(".");
    
    if (Serial1.available() > 0) {
        uint8 incomingByte = (uint8)Serial1.read();
        asebaBuffer.appendBuffer(&incomingByte, 1);
        Serial.print(incomingByte, HEX);
    }
    else if (millis() - tick > 10000) {
        Serial.print("sending ETX..." + String(tick) + "...");
        Serial1.write(ETX);
        Serial1.flush();
    }
    tick = millis();

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
        Serial1.write(ACK);
        Serial1.flush();
        Serial.print("ACK: ");
    }

    digitalWrite(led, HIGH);
}
