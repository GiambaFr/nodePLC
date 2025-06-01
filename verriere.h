#ifndef VERRIERE_H
#define VERRIERE_H


#include <nlohmann/json.hpp>
#include <vector>

#include "dmx.h"
#include "commons.h"

#include "withSingleThread.h"
#include "withConfig.h"
#include "withMqtt.h"

using json = nlohmann::json;

class VERRIERE: public withConfig<CONF::Light>, public withSingleThread, public withMqtt {
    private:
        
    public:
        VERRIERE();
        ~VERRIERE();


        void process();
        void _onMainThreadStart();
        void _onMainThreadStopping();
        void _onMainThreadStop();

}

#endif