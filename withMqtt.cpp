#include "myMQTT.h"

#include "withMqtt.h"

class withMqtt;

withMqtt::withMqtt(MyMqtt *myMqtt): myMqtt(myMqtt) {}

withMqtt::withMqtt() {}

void withMqtt::setMqtt(MyMqtt *myMqtt) {
    this->myMqtt = myMqtt;
}

MyMqtt *withMqtt::getMqtt() {
    return this->myMqtt;
}

void withMqtt::sendBack(std::string topic, json j) {
    this->myMqtt->publish(topic, j, false);
}

withMqtt::~withMqtt() {}