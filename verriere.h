#pragma once
#ifndef VERRIERE_H
#define VERRIERE_H


#include <nlohmann/json.hpp>
#include <vector>

#include "dmx.h"
#include "commons.h"
#include "digital_output.h"

#include "withConfig.h"
#include "withMqtt.h"
#include "withSingleThread.h"
#include "withState.h"

using json = nlohmann::json;

class VERRIERE: public withConfig<CONF::Verriere>, public withSingleThread, public withState<VERRIERE_STATE>, public withMqtt {
    private:

        Digital_Output* outputUp;
        Digital_Output* outputDown;

        bool init;

        //todo Analog_Input;

        /* --- FROM CONFIG --- */
        /*
            std::string name;
            std::string comment;
            long open_duration_ms;
            long close_duration_ms;
            long slowdown_duration_ms;
            int current_position; // 1 to 100
            std::string up_DoName;
            std::string down_DoName;
            std::string rain_sensor_AiName;
            std::string get_TOPIC;
            std::string set_TOPIC;
            std::string dispatch_TOPIC;
        */

        VERRIERE_STATE state;
        int current_position;
        int target_position;
        std::chrono::steady_clock::time_point actual_time;
        std::chrono::steady_clock::time_point motion_start_time;
        std::chrono::steady_clock::time_point last_mqtt_publish_time;

        using positionChangeHandlersFunc = std::function<void(int oldPosition, int newPosition)>;
        std::vector<positionChangeHandlersFunc> positionChangeHandlers;
        void _onPositionChange(int oldPosition, int newPosition);
        void addPositionChangeHandler(positionChangeHandlersFunc /*function*/);

        using targetChangeHandlersFunc = std::function<void(int oldTarget, int newTarget)>;
        std::vector<targetChangeHandlersFunc> targetChangeHandlers;
        void _onTargetChange(int oldPosition, int newPosition);
        void addTargetChangeHandler(targetChangeHandlersFunc /*function*/); 

        // Nouvelle fonction pour calculer la position basée sur le temps et le mouvement
        int calculateCurrentPositionBasedOnTime();


    public:
        VERRIERE(CONFIG* /*config*/, CONF::Verriere* /*lightConf*/, MyMqtt* /*myMqtt*/, Digital_Output*, Digital_Output* /*, TODO:: Analog_Input*/);
        ~VERRIERE();

        int getOpenDurationMs();
        void setOpenDurationMs(int duration);
        int getCloseDurationMs();
        void setCloseDurationMs(int duration);
        int getOpenSlowdownDurationMs();
        void setOpenSlowdownDurationMs(int duration);
        int getCloseSlowdownDurationMs();
        void setCloseSlowdownDurationMs(int duration);
        int getCurrentPosition();
        void setCurrentPosition(int position);
        int getTargetPosition();
        void setTargetPosition(int position);
        /*std::string getOutputUp();
        std::string getOutputDown();
        std::string getAIRainSensor();*/
        int timeToPercent(int totalTime, long timeMs);
        long percentToTime(int totalTime, int percent);


        
        void process();
        void _onMainThreadStart();
        void _onMainThreadStopping();
        void _onMainThreadStop();

};



class VERRIERES {
    private:
        std::vector<VERRIERE*> verrieres;


    public:
        VERRIERES(CONFIG* /*config*/, MyMqtt*, Digital_Outputs*);
        ~VERRIERES();

        void addVerriere(VERRIERE*);
        VERRIERE *findByName(std::string);
        void dump();

        void startChildrenThreads();
        void stopChildrenThreads();
        void joinChildrenThreads();

};

#endif