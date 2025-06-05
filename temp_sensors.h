#pragma once
#ifndef SENSOR_H
#define SENSOR_H

#include <iostream>
#include <vector>
#include <functional>
#include <thread>

#include <nlohmann/json.hpp>

#include "withSingleThread.h"
#include "withConfig.h"
#include "withMqtt.h"

#define TEMP_SENSOR_PATHTEMPLATE "/uncached/28.%s/temperature11"

#define BAD_VALUE (-100.0)

using json = nlohmann::json;

class Temp_Sensor: public withConfig<CONF::OW_Sensor>, public withSingleThread, public withMqtt {
    private:

        float value;

        std::string sensorPath;

        float _read();

        using temperatureChangeHandlersFunc = std::function<void(float,float)>;
        std::vector<temperatureChangeHandlersFunc> temperatureChangeHandlers;
        void _onTemperatureChange(float oldValue, float newValue);

    public:
        Temp_Sensor(CONFIG* /*config*/, CONF::OW_Sensor* /*sensorConf*/, MyMqtt* /*myMqtt*/);
        ~Temp_Sensor();

        std::string getAddress();
        void setAddress(std::string);

        float getMin();
        void setMin(float);
        float getMax();
        void setMax(float);
        float getAlarm();
        void setAlarm(float);

        float getValue();

        void process();
        void _onMainThreadStart();
        void _onMainThreadStopping();
        void _onMainThreadStop();

        void addTemperatureChangeHandler(temperatureChangeHandlersFunc function);
    
};



class Temp_Sensors {
    private:
        std::vector<Temp_Sensor*> sensors;
    public:
        Temp_Sensors(CONFIG* /*config*/, MyMqtt*);
        ~Temp_Sensors();
        
        void addTempSensor(Temp_Sensor*);
        Temp_Sensor *findByName(std::string);
        void dump();
        
        void startChildrenThreads();
        void stopChildrenThreads();
        void joinChildrenThreads();
};

#endif