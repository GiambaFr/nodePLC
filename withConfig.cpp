#include "withConfig.h"

template<typename ConfigT>
class withConfig;


template<typename ConfigT>
withConfig<ConfigT>::withConfig() {}

template<typename ConfigT>
withConfig<ConfigT>::withConfig(CONFIG *config, ConfigT *configT): config(config), configT(configT) {
}

template<typename ConfigT>
void withConfig<ConfigT>::setConfig(CONFIG *config) {
    this->config = config;
}

template<typename ConfigT>
CONFIG *withConfig<ConfigT>::getConfig() {
    return this->config;
}

template<typename ConfigT>
void withConfig<ConfigT>::setConfigT(ConfigT *configT) {
    this->configT = configT;
}

template<typename ConfigT>
ConfigT *withConfig<ConfigT>::getConfigT() {
    return this->configT;
}

template<typename ConfigT>
void withConfig<ConfigT>::saveConfig() {
    this->config->save();
}

template<typename ConfigT>
std::string withConfig<ConfigT>::getName() {
    return this->getConfigT()->name;
}

template<typename ConfigT>
void withConfig<ConfigT>::setName(std::string name) {
    this->getConfigT()->name = name;
    this->saveConfig();    
}

template<typename ConfigT>
std::string withConfig<ConfigT>::getComment() {
    return this->getConfigT()->comment;
}

template<typename ConfigT>
void withConfig<ConfigT>::setComment(std::string comment) {
    this->getConfigT()->comment = comment;
    this->saveConfig();    
}

template<typename ConfigT>
std::string withConfig<ConfigT>::getGetTOPIC() {
    return this->getConfigT()->get_TOPIC;    
}

template<typename ConfigT>
void withConfig<ConfigT>::setGetTOPIC(std::string t) {
    this->getConfigT()->get_TOPIC = t;
    this->saveConfig();    
}
 

template<typename ConfigT>
std::string withConfig<ConfigT>::getSetTOPIC() {
    return this->getConfigT()->set_TOPIC;    
}

template<typename ConfigT>
void withConfig<ConfigT>::setSetTOPIC(std::string t) {
    this->getConfigT()->set_TOPIC = t;
    this->saveConfig();    
}
 
template<typename ConfigT>
std::string withConfig<ConfigT>::getDispatchTOPIC() {
    return this->getConfigT()->dispatch_TOPIC;    
}

template<typename ConfigT>
void withConfig<ConfigT>::setDispatchTOPIC(std::string t) {
    this->getConfigT()->dispatch_TOPIC = t;
    this->saveConfig();    
}

template<typename ConfigT>
template<typename ParamT>
json  withConfig<ConfigT>::getSendBackParams(std::string param, ParamT value) {
    json j;
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    j[param] = value;
    return j;
}
/*
* take json from mqtt
* use to set params:
* - name
* - comment
* - setTOPIC
* - dispatchTOPIC
*/
template<typename ConfigT>
void withConfig<ConfigT>::setBaseParams(json params) {
    if (params.contains("name") && !params["name"].empty()) {
        this->setName(params["name"].get<std::string>());
    }
    if (params.contains("comment") && !params["comment"].empty()) {
        this->setComment(params["comment"].get<std::string>());
    }
    if (params.contains("setTOPIC") && !params["setTOPIC"].empty()) {
        this->setSetTOPIC(params["setTOPIC"].get<std::string>());
    }
    if (params.contains("getTOPIC") && !params["getTOPIC"].empty()) {
        this->setGetTOPIC(params["getTOPIC"].get<std::string>());
    }
    if (params.contains("dispatchTOPIC") && !params["dispatchTOPIC"].empty()) {
        this->setDispatchTOPIC(params["dispatchTOPIC"].get<std::string>());
    }
}

template<typename ConfigT>
withConfig<ConfigT>::~withConfig() {

}

template class withConfig<CONF::Input>;
template class withConfig<CONF::Output>;
template class withConfig<CONF::OW_Sensor>;
template class withConfig<CONF::Light>;
template class withConfig<CONF::Verriere>;
template class withConfig<CONF::Analog_Input>;