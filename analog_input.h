#pragma once
#ifndef AINPUT_H
#define AINPUT_H

#include <iostream>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>

#include <nlohmann/json.hpp>

#include "withSingleThread.h"
#include "withConfig.h"
#include "withMqtt.h"

/*
bit 3-2 S1-S0: Sample Rate Selection Bit
0 00 = 240 SPS (12 bits) (Default)
1 01 = 60 SPS (14 bits)
2 10 = 15 SPS (16 bits)
3 11 = 3.75 SPS (18 bits)

bit 1-0 G1-G0: PGA Gain Selection Bits
0 00 = x1 (Default)
1 01 = x2
2 10 = x4
3 11 = x8

Resolution Setting LSB
12 bits 1 mV
14 bits 250 µV
16 bits 62.5 µV
18 bits 15.625 µV
*/

/*VELUX*/
/*
noir : +3.3v - gris: 0v (gnd) - violet: signal de 0 à +3.3v 
*/

#define MCP3422_PIN_BASE 90

using json = nlohmann::json;

class Analog_Input: public withConfig<CONF::Analog_Input>, public withSingleThread, public withMqtt {
    private:

        int fd;
        float lsb;

        float value;

        std::mutex *i2cMutex;
        float _read();

        using voltageChangeHandlersFunc = std::function<void(float,float)>;
        std::vector<voltageChangeHandlersFunc> voltageChangeHandlers;
        void _onVoltageChange(float oldValue, float newValue);

    public:
        Analog_Input(CONFIG* /*config*/, CONF::Analog_Input* /*sensorConf*/, MyMqtt* /*myMqtt*/, int /*fd*/,float /*lsb*/, std::mutex *i2cMutex);
        ~Analog_Input();

        int getChannel();
        float getK();

        float getValue();

        void process();
        void _onMainThreadStart();
        void _onMainThreadStopping();
        void _onMainThreadStop();

        void addVoltageChangeHandler(voltageChangeHandlersFunc function);
    
};



class Analog_Inputs {
    private:
        std::vector<Analog_Input*> inputs;
        int fd;
        std::mutex i2cMutex;
    public:
        Analog_Inputs(CONFIG* /*config*/, MyMqtt*);
        ~Analog_Inputs();

        void addChannel(Analog_Input*);
        Analog_Input *findByName(std::string);
        void dump();
        
        void startChildrenThreads();
        void stopChildrenThreads();
        void joinChildrenThreads();
};

#endif