#ifndef DIGITAL_OUTPUT
#define DIGITAL_OUTPUT
#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <wiringPi.h>


#include "commons.h"
#include "withSingleThread.h"
#include "withConfig.h"
#include "withMqtt.h"
#include "withState.h"

#define MCP23008_ADDRESS 0x20
#define MCP23008_EXT_ADDRESS 0x24

#define MCP23008_PIN_BASE 64  // 8 bit relays

using json = nlohmann::json;

class Digital_Outputs;

class Digital_Output: public withSingleThread, public withConfig<CONF::Output>, public withState<STATE>, public withMqtt {
    private:
        Digital_Outputs* DOs;

        STATE _read();
        std::vector<std::string> locks; // set all locks to 0 before set this to 1

        

        void _write(STATE);


    public:
        Digital_Output(CONFIG * /*config*/, CONF::Output* /*outputConf*/, MyMqtt* /*myMqtt*/, Digital_Outputs* /*DOs*/);
        ~Digital_Output();

        int getPin();
        void setPin(int);

        void setForced(bool);
        bool isForced();
        STATE getForcedValue();
        void setForcedValue(STATE);
        void setRetain(bool);
        bool isRetain();
        STATE getRetainValue();
        void setRetainValue(STATE);

        bool isDimmable();

        void addLock(std::string /*lock*/);
        bool hasLock();
        std::vector<std::string> getLocks();
        

        void toggle();

        void process();
        void _onMainThreadStart();
        void _onMainThreadStopping();
        void _onMainThreadStop();

};

class Digital_Outputs {
    private:
        std::vector<Digital_Output*> outputs;


    public:
        Digital_Outputs(CONFIG* /*config*/, MyMqtt*);
        ~Digital_Outputs();

        void addOutput(Digital_Output*);
        Digital_Output *findByName(std::string);
        void dump();

        void startChildrenThreads();
        void stopChildrenThreads();
        void joinChildrenThreads();

        /*void startThread();
        void stopThread();
        void joinThread();*/
};

#endif
