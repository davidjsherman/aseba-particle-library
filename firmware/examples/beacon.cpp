#include "AsebaLite.h"
//#include "Monitor/Monitor.h"

#define MAX_VM_DATA 40

int led = D7; // built-in LED
int phi = D0;

sint16 tick = -1;
static std::vector<sint16> vm_data;

//Monitor monitor;
TCPServer aseba_tcp = TCPServer(33339);
TCPClient tcp_client;

AsebaLite::AsebaBuffer asebaBuffer;

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
    pinMode(led, OUTPUT);     // initialize D7 pin as digital output
    pinMode(phi, OUTPUT);    // initialize D0 pin as digital output
    digitalWrite(led, LOW);
    digitalWrite(phi, LOW);
    
    Serial.begin(9600);       // usb monitor
    Serial1.begin(9600);      // tx/rx pins
    //    monitor.ledOff();
    //    monitor.begin();
    aseba_tcp.begin();

    //digitalWrite(led, HIGH);
    Spark.variable("beacon_tick",  &tick, INT); // expose message as cloud variable
    //    monitor.variable("beacon_tick",&tick, INT);
}

void loop() {
    if (WiFi.ready()) {
        Spark.publish("beacon","tick "+String(tick));
        Spark.publish("beacon","wifi "+String(WiFi.SSID())+" "+String(WiFi.localIP()));
        Serial.println(WiFi.localIP());
    }

    digitalWrite(phi, HIGH);
    tcp_client = aseba_tcp.available();
    if (tcp_client.connected()) {
        Serial.println("client "+String(tcp_client));
        Spark.publish("beacon","client "+String(tcp_client));
    }
    digitalWrite(phi, LOW);

    //    monitor.report();

    digitalWrite(led, LOW);
    delay(1000);
    tick += 1;
    digitalWrite(led, HIGH);
    
    vm_data.clear();
    for (unsigned int i=0; i<MAX_VM_DATA; i++) {
        vm_data.push_back( (sint16)(64 - i) );
    }
    
    if ((tick % 5) == 0) {
        // emit user event
        vm_data[35] = tick;
        asebaBuffer.messageUserMessage(0, std::vector<sint16>(vm_data.begin(),vm_data.begin()+36));
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("   0 user message from 1 : user message of size 36 : " + tick);
    }
    if ((tick % 5) == 0) {
        // get var
        asebaBuffer.messageGetVariables(0, 36);
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0xa00b get variables from 0 : dest 1 start 0, length 36");
        // send var
        asebaBuffer.messageVariables(0, std::vector<sint16>(vm_data.begin(),vm_data.begin()+36));
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0x9005 variables from 1 : start 0, variables vector of size 36");
    }
    if ((tick % 30) == 0) {
        // set var
        std::vector<sint16> tick_vec = { tick };
        asebaBuffer.messageSetVariables(35, tick_vec);
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0xa00c set variables from 0 : dest 2 start 35, variables vector of size 1");
    }
    if ((tick % 180) == 0) {
        // send description
        asebaBuffer.messageDescription("beacon", 4, 512, 64, 1059, 4, 1, 1);
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0x9000 description from 1 : Node beacon using protocol version 4");
        Serial.println("bytecode: 512, stack: 64, variables: 1059");
        Serial.println("named variables: 4");
        Serial.println("local events: 1");
        Serial.println("native functions: 1");

        asebaBuffer.messageNamedVariableDescription(1,"id");
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0x9001 named variable from 1 : id of size 1");
        asebaBuffer.messageNamedVariableDescription(1,"source");
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0x9001 named variable from 1 : source of size 1");
        asebaBuffer.messageNamedVariableDescription(32,"args");
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0x9001 named variable from 1 : args of size 32");
        asebaBuffer.messageNamedVariableDescription(1,"_productId");
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0x9001 named variable from 1 : _productId of size 1");
        
        asebaBuffer.messageLocalEventDescription("timer","periodic timer at 50 Hz");
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0x9002 local event from 1 : timer : periodic timer at 50 Hz");
        
        std::vector<sint16> psize = { -1, -1 };
        std::vector<String> pname = { "dest", "src" };
        asebaBuffer.messageNativeFunctionDescription("math.copy","copies src to dest element by element",2, psize, pname);
        asebaBuffer.writeBufferAll(aseba_tcp);
        Serial.println("0x9003 native function description from 1 : math.copy (dest<-1>, src<-1>) : copies src to dest element by element");
    }
    
}
