#include <iostream>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>

#include "commons.h"
#include "config.h"

using json = nlohmann::json;

namespace CONF {


    void to_json(json& j, const Config& c) {
        j["MQTT"] = c.mqtt->getJsonConfig(c.mqtt);
        j["Digital_Inputs"] = c.inputs->getJsonConfig(c.inputs);
        j["Digital_Outputs"] = c.outputs->getJsonConfig(c.outputs);
        j["OW"] = c.ow->getJsonConfig(c.ow);
        j["Lights"] = c.lights->getJsonConfig(c.lights);
        j["Verrieres"] = c.verrieres->getJsonConfig(c.verrieres);
    }

    void from_json(const json& j, Config& c) {
        c.mqtt = new CONF::Mqtt();
        c.mqtt->clientId = j["MQTT"]["clientId"].get<std::string>();
        c.mqtt->host = j["MQTT"]["host"].get<std::string>();
        c.mqtt->user = j["MQTT"]["user"].get<std::string>();
        c.mqtt->password = j["MQTT"]["password"].get<std::string>();
        c.mqtt->willTopic = j["MQTT"]["willTopic"].get<std::string>();
        c.mqtt->willMessage = j["MQTT"]["willMessage"].get<std::string>();
        c.mqtt->onlineMessage = j["MQTT"]["onlineMessage"].get<std::string>();
        for(auto& st: j["MQTT"]["suscribeTopics"]) {
            c.mqtt->suscribeTopics.push_back(st.get<std::string>());
        }

        c.inputs = new CONF::Digital_Inputs();
        for(auto& i: j["Digital_Inputs"]["Inputs"]) {
            Input *input = new CONF::Input();
            input->name = i["name"].get<std::string>();
            input->comment = i["comment"].get<std::string>();
            input->pin = i["pin"].get<int>();
            input->set_TOPIC = i["set_TOPIC"].get<std::string>();
            input->get_TOPIC = i["get_TOPIC"].get<std::string>();
            input->dispatch_TOPIC = i["dispatch_TOPIC"].get<std::string>();
            input->isButton = i["isButton"].get<bool>();
            input->buttonDispatch_TOPIC = i["buttonDispatch_TOPIC"].get<std::string>();
            input->button.isButton = i["button"]["isButton"].get<bool>();
            if (input->button.isButton == true) {
                input->button.buttonDispatch_TOPIC = i["button"]["buttonDispatch_TOPIC"].get<std::string>();
                input->button.DC_GAP = i["button"]["DC_GAP"].get<int>();
                input->button.HOLD_TIME = i["button"]["HOLD_TIME"].get<int>();
            }
            c.inputs->inputs.push_back(input);
        }
        c.outputs = new CONF::Digital_Outputs();
        for(auto& o: j["Digital_Outputs"]["Outputs"]) {
            Output *output = new CONF::Output();
            output->name = o["name"].get<std::string>();
            output->comment = o["comment"].get<std::string>();
            output->pin = o["pin"].get<int>();
            output->forced = o["forced"].get<bool>();
            output->forcedValue = o["forcedValue"].get<std::string>();
            output->retain = o["retain"].get<bool>();
            output->retainValue = o["retainValue"].get<std::string>();
            output->set_TOPIC = o["set_TOPIC"].get<std::string>();
            output->get_TOPIC = o["get_TOPIC"].get<std::string>();
            output->dispatch_TOPIC = o["dispatch_TOPIC"].get<std::string>();
            //output->dimmable = o["dimmable"].get<bool>();
            output->type = outputType_from_string(o["type"].get<std::string>());
            if(o["locks"].is_array() && !o["locks"].empty()) {
                for (auto & l: o["locks"]) {
                    output->locks.push_back(l.get<std::string>());
                }
            }
            c.outputs->outputs.push_back(output);
        }
        c.ow = new CONF::OW();
        c.ow->server = j["OW"]["server"].get<std::string>();
        c.ow->port = j["OW"]["port"].get<int>();
        for(auto& s: j["OW"]["1W_TEMP_SENSOR"]) {
            OW_Sensor *sensor = new CONF::OW_Sensor();
            sensor->name = s["name"].get<std::string>();
            sensor->comment = s["comment"].get<std::string>();
            sensor->address = s["address"].get<std::string>();
            sensor->max = s["max"].get<float>();
            sensor->min = s["min"].get<float>();
            sensor->alarm = s["alarm"].get<float>();
            sensor->set_TOPIC = s["set_TOPIC"].get<std::string>();
            sensor->get_TOPIC = s["get_TOPIC"].get<std::string>();
            sensor->dispatch_TOPIC = s["dispatch_TOPIC"].get<std::string>();
            c.ow->sensors.push_back(sensor);
        }
        
        c.lights = new CONF::Lights();
        for(auto& l: j["Lights"]["Lights"]) {
            Light *light = new CONF::Light();
            light->name = l["name"].get<std::string>();
            light->comment = l["comment"].get<std::string>();
            light->type = lightType_from_string(l["type"].get<std::string>());
            light->current_value = l["current_value"].get<int>();
            light->set_TOPIC = l["set_TOPIC"].get<std::string>();
            light->get_TOPIC = l["get_TOPIC"].get<std::string>();
            light->dispatch_TOPIC = l["dispatch_TOPIC"].get<std::string>();
            if(light->type == LIGHT_TYPE::DIMMABLE) {
                light->memo_value = l["memo_value"].get<int>();
                light->increment = l["increment"].get<int>();
                light->channel = l["channel"].get<int>();
            } else if(light->type == LIGHT_TYPE::ONOFF) {
                light->output_TOPIC = l["output_TOPIC"].get<std::string>();
                light->memo_value = 0;
                light->increment = 0;
            }
            c.lights->lights.push_back(light);
        }

        c.verrieres = new CONF::Verrieres();
        for(auto& v: j["Verrieres"]["Verrieres"]) {
            Verriere *verriere = new CONF::Verriere();
            verriere->name = v["name"].get<std::string>();
            verriere->comment = v["comment"].get<std::string>();
            verriere->set_TOPIC = v["set_TOPIC"].get<std::string>();
            verriere->get_TOPIC = v["get_TOPIC"].get<std::string>();
            verriere->dispatch_TOPIC = v["dispatch_TOPIC"].get<std::string>();
            verriere->open_duration_ms = v["open_duration_ms"].get<int>();
            verriere->close_duration_ms = v["close_duration_ms"].get<int>();
            verriere->open_slowdown_duration_ms = v["open_slowdown_duration_ms"].get<int>();
            verriere->close_slowdown_duration_ms = v["close_slowdown_duration_ms"].get<int>();
            verriere->current_position = v["current_position"].get<int>();
            verriere->up_DoName = v["up_DoName"].get<std::string>();
            verriere->down_DoName = v["down_DoName"].get<std::string>();
            verriere->rain_sensor_AiName = v["rain_sensor_AiName"].get<std::string>();
            c.verrieres->verrieres.push_back(verriere);
        }


    }
    
}


json CONF::Mqtt::getJsonConfig(CONF::Mqtt *mqtt){
    json j = json{{"host", mqtt->host}, {"clientId", mqtt->clientId}, {"willTopic", mqtt->willTopic}, {"willMessage", mqtt->willMessage}, {"onlineMessage", mqtt->onlineMessage}, {"user", mqtt->user}, {"password", mqtt->password}};
    j["suscribeTopics"] = json::array();
    for (auto& st:mqtt->suscribeTopics) {
        j["suscribeTopics"].push_back(st);
    }
    return j;
}

json CONF::Input::getJsonConfig(CONF::Input *input){
    json i;
    i["name"] = input->name;
    i["comment"] = input->comment;
    i["pin"] = input->pin;
    i["get_TOPIC"] = input->get_TOPIC;
    i["set_TOPIC"] = input->set_TOPIC;
    i["dispatch_TOPIC"] = input->dispatch_TOPIC;
    i["isButton"] = input->isButton;
    i["buttonDispatch_TOPIC"] = input->buttonDispatch_TOPIC;
    //i["button"] = json::object();
    i["button"]["isButton"] = input->button.isButton;
    if (input->button.isButton == true) {
        i["button"]["buttonDispatch_TOPIC"] = input->button.buttonDispatch_TOPIC;
        i["button"]["DC_GAP"] = input->button.DC_GAP;
        i["button"]["HOLD_TIME"] = input->button.HOLD_TIME;
    }
    return i;
}

json CONF::Digital_Inputs::getJsonConfig(CONF::Digital_Inputs *i){
    json j;
    j["Inputs"] = json::array();
    for(auto& input: i->inputs) {
        j["Inputs"].push_back(input->getJsonConfig(input));
    }
    return j;
}

json CONF::Output::getJsonConfig(CONF::Output *output){
    json o;
    o["name"] = output->name;  
    o["comment"] = output->comment;       
    o["pin"] = output->pin;
    o["forced"] = output->forced;
    o["forcedValue"] = output->forcedValue;
    o["retain"] = output->retain;
    o["retainValue"] = output->retainValue;
    o["get_TOPIC"] = output->get_TOPIC;
    o["set_TOPIC"] = output->set_TOPIC;
    o["dispatch_TOPIC"] = output->dispatch_TOPIC;
    //o["dimmable"] = output->dimmable;
    o["type"] = outputType_to_string(output->type);
    o["locks"] = json::array();
    for (auto& lock: output->locks) {
        o["locks"].push_back(lock);
    }
    return o;
}

json CONF::Digital_Outputs::getJsonConfig(CONF::Digital_Outputs *o){
    json j;
    j["Outputs"] = json::array();
    for(auto& output: o->outputs) {
        j["Outputs"].push_back(output->getJsonConfig(output));
    }
    return j;
}


json CONF::OW_Sensor::getJsonConfig(CONF::OW_Sensor *sensor){
    json s;
    s["name"] = sensor->name;
    s["comment"] = sensor->comment;
    s["address"] = sensor->address;
    s["min"] = sensor->min;
    s["max"] = sensor->max;
    s["alarm"] = sensor->alarm;
    s["get_TOPIC"] = sensor->get_TOPIC;
    s["set_TOPIC"] = sensor->set_TOPIC;
    s["dispatch_TOPIC"] = sensor->dispatch_TOPIC;
    return s;
}

json CONF::OW::getJsonConfig(CONF::OW *ow){
    json j;
    j["server"] = ow->server;
    j["port"] = ow->port;
    j["1W_TEMP_SENSOR"] = json::array();    
    for(auto& sensor: ow->sensors) {
        j["1W_TEMP_SENSOR"].push_back(sensor->getJsonConfig(sensor));
    }
    return j;
}

json CONF::Light::getJsonConfig(CONF::Light *light){
    json l;
    l["name"] = light->name;
    l["comment"] = light->comment;
    l["current_value"] = light->current_value;
    l["type"] = lightType_to_string(light->type);
    l["get_TOPIC"] = light->get_TOPIC;
    l["set_TOPIC"] = light->set_TOPIC;
    l["dispatch_TOPIC"] = light->dispatch_TOPIC;
    if (light->type == LIGHT_TYPE::DIMMABLE) { //dimmableLights
        l["channel"] = light->channel;
        l["memo_value"] = light->memo_value;
        l["increment"] = light->increment;
    } else if (light->type == LIGHT_TYPE::ONOFF) { //for switchLights
        l["output_TOPIC"] = light->output_TOPIC; 
    }
    return l;
}

json CONF::Lights::getJsonConfig(CONF::Lights *lights){
    json j;
    j["Lights"] = json::array();
    for(auto& light: lights->lights) {
        j["Lights"].push_back(light->getJsonConfig(light));
    }
    return j;
}


json CONF::Verriere::getJsonConfig(CONF::Verriere *verriere){
    json v;
    v["name"] = verriere->name;
    v["comment"] = verriere->comment;
    v["open_duration_ms"] = verriere->open_duration_ms;
    v["close_duration_ms"] = verriere->close_duration_ms;
    v["open_slowdown_duration_ms"] = verriere->open_slowdown_duration_ms;
    v["close_slowdown_duration_ms"] = verriere->close_slowdown_duration_ms;
    v["up_DoName"] = verriere->up_DoName;
    v["down_DoName"] = verriere->down_DoName;
    v["rain_sensor_AiName"] = verriere->rain_sensor_AiName;
    v["current_position"] = verriere->current_position;
    v["get_TOPIC"] = verriere->get_TOPIC;
    v["set_TOPIC"] = verriere->set_TOPIC;
    v["dispatch_TOPIC"] = verriere->dispatch_TOPIC;
    return v;
}


json CONF::Verrieres::getJsonConfig(CONF::Verrieres *verrieres){
    json j;
    j["Verrieres"] = json::array();
    for(auto& verriere: verrieres->verrieres) {
        j["Verrieres"].push_back(verriere->getJsonConfig(verriere));
    }
    return j;
}

CONFIG::CONFIG() {

}

CONF::Config *CONFIG::getConfig() {
    return &this->config;
}

void CONFIG::load(std::string fileName) {
    this->fileName = fileName;
    std::ifstream configFile(this->fileName);
    if (!configFile.is_open()) {
        std::cerr << "Error opening config file: " << this->fileName << std::endl;
        // Optionally, create a default config or throw an exception
        return;
    }
    json j;
    configFile >> j;
    this->config = j.get<CONF::Config>();   
}

void CONFIG::save() {
    json j = this->config;
    std::ofstream file(this->fileName);
    file << std::setw(4) << j << std::endl;
}

CONFIG::~CONFIG() {}