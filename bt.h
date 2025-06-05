#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <functional> 

#include "commons.h"

//#define DC_GAP (200)
//#define HOLD_TIME (500)

class BT {
    private:
        STATE lastState;
        //STATE curState;

        std::chrono::steady_clock::time_point pressedTime;
        std::chrono::steady_clock::time_point releasedTime;
        std::chrono::steady_clock::time_point t0;
        std::chrono::steady_clock::time_point t1;

        unsigned int clickCount;
        CLICK_TYPE clickType;


        using actionHandlersFunc = std::function<void(BUTTON_ACTION, unsigned int)>;
        std::vector<actionHandlersFunc> actionHandlers;
        void _onAction(BUTTON_ACTION btAction, unsigned int clickCount);
    public:
        BT();
        ~BT();

        int dcGap;
        int holdTime;

        void process(STATE curState);

        void addActionHandler(actionHandlersFunc function);

};