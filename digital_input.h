#pragma once
#ifndef DIGITAL_INPUT
#define DIGITAL_INPUT

#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <wiringPi.h>

#include "commons.h"
#include "bt.h"

#include "withConfig.h"
#include "withMqtt.h"
#include "withSingleThread.h"
#include "withState.h"

using json = nlohmann::json;

class Digital_Input: public withSingleThread, public withConfig<CONF::Input>, public withMqtt, withState<STATE> {
    private:
        STATE _read();
        BT *button = nullptr;
    public:
        Digital_Input(CONFIG* /*config*/, CONF::Input* /*inputConf*/, MyMqtt* /*myMqtt*/);
        ~Digital_Input();

        int getPin();
        void setPin(int);

        void process();

        void _onMainThreadStart();
        void _onMainThreadStopping();
        void _onMainThreadStop();

        bool isButton();
        std::string getButtonDispatchTOPIC();

};

class Digital_Inputs {
    private:
        std::vector<Digital_Input*> inputs;
    public:
        Digital_Inputs(CONFIG* /*config*/, MyMqtt*);
        ~Digital_Inputs();
        void addInput(Digital_Input*);
        Digital_Input *findByName(std::string);
        void dump();
        void startChildrenThreads();
        void stopChildrenThreads();
        void joinChildrenThreads();
};

#endif
