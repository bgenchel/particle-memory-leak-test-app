#include "application.h" //needed when compiling spark locally
#include "WebInterface.h"
#include "MessageSenderOne.h"
#include "MessageSenderTwo.h"

using namespace std;

SYSTEM_MODE(MANUAL);

WebInterface wi;
MessageSenderOne ms1(&wi);
MessageSenderTwo ms2(&wi);

int FREE_MEMORY;

void check_set_free_memory(){
    if(FREE_MEMORY - System.freeMemory() > 100){
        Serial.println("###########################");
        Serial.println("# DROP IN MEMORY");
        Serial.println("###########################");
    }
    FREE_MEMORY = System.freeMemory();
}

void setup()
{
    FREE_MEMORY = System.freeMemory();
    Serial.println("Main::Setup:: Waiting ...");
    delay(2000);

    Serial.println("Turning on wifi module");
    WiFi.on();
    Serial.println("Attempting to connect to wifi");
    WiFi.connect();
    Serial.println("Checking connection");
    while(!WiFi.ready()){
        Serial.println("wifi connection failed, retrying ...");
        delay(3000);
        WiFi.connect();
    }
    Serial.println("WiFi connection succeeded!");

    Serial.println("Attempting to connect to host");
    if(wi.TCPConnect()){
        Serial.println("Connected to host");
    } else {
        Serial.println("Unable to connect to host");
    }
    delay(2000);
}

void loop()
{
    ms1.Run(20);
    check_set_free_memory();
    ms2.Run(20);
    check_set_free_memory();
    wi.Run(20);
    check_set_free_memory();
}
