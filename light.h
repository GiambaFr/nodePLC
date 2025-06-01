#ifndef LIGHT_H
#define LIGHT_H


#include <nlohmann/json.hpp>
#include <vector>

#include "dmx.h"
#include "commons.h"

#include "withSingleThread.h"
#include "withConfig.h"
#include "withMqtt.h"

using json = nlohmann::json;


class LIGHT: public withConfig<CONF::Light>, public withSingleThread, public withMqtt {
    private:
        int value;
        bool dimming;
        int updown; // 1 / -1 multiplicator

        using valueChangeHandlersFunc = std::function<void(int /*last_value*/, int /*value*/)>;
        std::vector<valueChangeHandlersFunc> valueChangeHandlers;
        void _onValueChange(int /*last_value*/, int /*value*/);
        std::vector<valueChangeHandlersFunc> memoValueChangeHandlers;
        void _onMemoValueChange(int /*last_value*/, int /*value*/);

    public:
        LIGHT(CONFIG* /*config*/, CONF::Light* /*lightConf*/, MyMqtt* /*myMqtt*/);
        ~LIGHT();


        void setIncrement(int /*increment*/);
        int getIncrement();

        LIGHT_TYPE getType();
        void setType(LIGHT_TYPE);

        void setChannel(int /*channel*/);
        int getChannel();

        void setValue(int /*value*/);
        int getValue();
        void setMemoValue(int /*value*/);
        int getMemoValue();



        void dimStart();
        void dimStop();

        std::string getOutputTOPIC();
        void setOutputTOPIC(std::string);

        void increment();
        void decrement();
        void toggle();

        void process();
        void _onMainThreadStart();
        void _onMainThreadStopping();
        void _onMainThreadStop();
        
        void addValueChangeHandler(valueChangeHandlersFunc /*function*/);
        void addMemoValueChangeHandler(valueChangeHandlersFunc /*function*/);
};

class LIGHTS {
    private:
        std::vector<LIGHT*> lights;
    public:
        LIGHTS(CONFIG* /*config*/, MyMqtt*);
        ~LIGHTS();

        void addLight(LIGHT*);
        LIGHT *findByName(std::string);
        void dump();
        void startChildrenThreads();
        void stopChildrenThreads();
        void joinChildrenThreads();
};

#endif