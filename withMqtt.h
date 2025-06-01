#ifndef WITH_MQTT_H
#define WITH_MQTT_H

#include "myMQTT.h"

class withMqtt {
    private:   
        MyMqtt *myMqtt;
    protected:
        void setMqtt(MyMqtt*);
        MyMqtt *getMqtt();
    public:
        withMqtt(MyMqtt*);
        withMqtt();
        virtual ~withMqtt();

        void sendBack(std::string /*topic*/, json);

};


#endif