#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <nlohmann/json.hpp>

#include "commons.h"

using json = nlohmann::json;

namespace CONF {

    typedef struct Mqtt {
        std::string clientId;
        std::string host;
        std::string willTopic;
        std::string willMessage;
        std::string onlineMessage;
        std::string user;
        std::string password;
        std::vector<std::string> suscribeTopics;
        json getJsonConfig(CONF::Mqtt*);
    } Mqtt;

    typedef struct Button {
        bool isButton;
        std::string buttonDispatch_TOPIC;
        int DC_GAP;
        int HOLD_TIME;
    } Button;

    typedef struct Input {
        std::string name;
        std::string comment;
        int pin;
        std::string get_TOPIC;
        std::string set_TOPIC;
        std::string dispatch_TOPIC;
        json getJsonConfig(CONF::Input*);
        bool isButton;
        std::string buttonDispatch_TOPIC;
        Button button;
    } Input;

    typedef struct Digital_Inputs {
        std::vector<Input*> inputs;
        json getJsonConfig(CONF::Digital_Inputs*);
    } Digital_Inputs;


    typedef struct Output {
        std::string name;
        std::string comment;
        int pin;
        bool forced;
        std::string forcedValue;
        bool retain;
        std::string retainValue;
        std::string get_TOPIC;
        std::string set_TOPIC;
        std::string dispatch_TOPIC;
        std::vector<std::string> locks;
        OUTPUT_TYPE type;
        json getJsonConfig(CONF::Output*);
    } Output;

    typedef struct Digital_Outputs {
        std::vector<Output*> outputs;
        json getJsonConfig(CONF::Digital_Outputs*);
    } Digital_Outputs;   


    typedef struct OW_Sensor {
        std::string name;
        std::string comment;
        std::string address;
        float min;
        float max;
        float alarm;
        std::string get_TOPIC;
        std::string set_TOPIC;
        std::string dispatch_TOPIC;
        json getJsonConfig(CONF::OW_Sensor*);
    } OW_Sensor;

    typedef struct OW {
        std::string server;
        int port;
        std::vector<OW_Sensor*> sensors;
        json getJsonConfig(CONF::OW*);
    } OW;


    typedef struct Light {
        std::string name;
        std::string comment;
        LIGHT_TYPE type;
        int channel;
        int current_value; //value on restart
        int memo_value; //value for dimmable toggle
        int increment;
        std::string get_TOPIC;
        std::string set_TOPIC;
        std::string dispatch_TOPIC;
        std::string output_TOPIC;
        json getJsonConfig(CONF::Light*);
    } Light;

    typedef struct Lights {
        std::vector<Light*> lights;
        json getJsonConfig(CONF::Lights*);
    } Lights;
 
    typedef struct Verriere {
        std::string name;
        std::string comment;
        long open_duration_ms;
        long close_duration_ms;
        long open_slowdown_duration_ms;
        long close_slowdown_duration_ms;
        int current_position; // 1 to 100
        std::string up_DoName;
        std::string down_DoName;
        std::string rain_sensor_AiName;
        std::string get_TOPIC;
        std::string set_TOPIC;
        std::string dispatch_TOPIC;
        json getJsonConfig(CONF::Verriere*);
    } Verriere;

    typedef struct Verrieres {
        std::vector<Verriere*> verrieres;
        json getJsonConfig(CONF::Verrieres*);
    } Verrieres;


    struct Config {
        Mqtt *mqtt;
        Digital_Inputs *inputs;
        Digital_Outputs *outputs;
        OW *ow;
        Lights *lights;
        Verrieres *verrieres;
    };

    void to_json(json& j, const Config& c);
    void from_json(const json& j, Config& c);

};



class CONFIG {
    private:
        std::string fileName;
        CONF::Config config;
    public:
        CONFIG();
        ~CONFIG();
        CONF::Config *getConfig();
        int load(std::string /*fileName*/);
        void save();
};

//extern CONFIG *config;

#endif