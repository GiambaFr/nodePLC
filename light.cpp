
#include <nlohmann/json.hpp>

#include "commons.h"
#include "light.h"

#include "threadPool.h"

using json = nlohmann::json;



const int DIMMABLE_MAX_VALUE = 255;

class LIGHT;

LIGHT::LIGHT(CONFIG *config, CONF::Light *lightConf, MyMqtt *myMqtt): withConfig(config, lightConf), withMqtt(myMqtt) {

    this->setThreadSleepTimeMillis(100);

    this->dimming = false;   
    this->updown = 1;

    //see output topic
    //this->setChannel(this->getConfigT()->channel);

    this->setValue(this->getConfigT()->current_value);   


    if (this->getType() == LIGHT_TYPE::DIMMABLE) {
        //add dim actions
        this->setIncrement(this->getConfigT()->increment);        
        this->setMemoValue(this->getConfigT()->memo_value);
        //save memo_value
        this->addMemoValueChangeHandler([this](int last_value, int value){
            json j;
            j["event"] = "MEMO_VALUE_CHANGE";
            j["name"] = this->getName();
            j["comment"] = this->getComment();
            j["value"] = this->getValue();
            j["memo_value"] = value;
            this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
        });
    }
    //send mqtt
    this->addValueChangeHandler([this](int last_value, int value){
        //std::cout << "last_val = " << last_value << ", new_val = " << value << std::endl << std::flush; 
        json j;
        j["event"] = "VALUE_CHANGE";
        j["name"] = this->getName();
        j["comment"] = this->getComment();
        j["value"] = value;
        j["memo_value"] = this->getMemoValue();
        //j["lightType"] = lightType_to_string(this->getType());
        this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
    });

    this->addValueChangeHandler([this](int last_value, int value){
        ///std::cout << "last_val = " << last_value << ", new_val = " << value << std::endl << std::flush; 
        if(this->getType() == LIGHT_TYPE::DIMMABLE)
            dmx_setChannelValue(this->getChannel(), value);
            //TODO: move dmx under mqtt then no need for channel
        else if (this->getType() == LIGHT_TYPE::ONOFF) {
            json j;
            j["state"] = state_to_string(value == 1 ? STATE::ON : STATE::OFF);
            this->getMqtt()->publish(this->getOutputTOPIC(), j, false);
        }
    });

    //dim/toggle..
    this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload){
            // HousePLC/LIGHTS/LIGHT_SALON/SET
            if (topic == this->getSetTOPIC()){
                //std::cout << payload << std::endl << std::flush;
                json j = json::parse(payload);
                if (j.contains("action") && !j["action"].empty()) {
                    LIGHT_ACTION action = lightAction_from_string(j["action"].get<std::string>());
                    switch(action) {
                        case LIGHT_ACTION::TOGGLE:
                            this->toggle();
                        break;
                        case LIGHT_ACTION::MAX:
                            if(this->getType() == LIGHT_TYPE::DIMMABLE)
                                this->setValue(DIMMABLE_MAX_VALUE);
                        break;
                        case LIGHT_ACTION::DIM_START:
                            if(this->getType() == LIGHT_TYPE::DIMMABLE)
                                this->dimStart();
                        break;
                        case LIGHT_ACTION::DIM_STOP:
                            if(this->getType() == LIGHT_TYPE::DIMMABLE)
                                this->dimStop();
                        break;
                        case LIGHT_ACTION::OFF:
                                this->setValue(0);
                        break;
                        case LIGHT_ACTION::ON:
                            if(this->getType() == LIGHT_TYPE::DIMMABLE)
                                this->setValue(this->getMemoValue());
                            else 
                                this->setValue(1);
                        break;
                        case LIGHT_ACTION::SET:
                            this->setValue(j["value"].get<int>());
                        break;
                        case LIGHT_ACTION::MEMO:
                            this->setMemoValue(this->getValue());
                        break;
                    }
                }

                //params
                this->setBaseParams(j);
                if (j.contains("increment") && !j["increment"].empty()) this->setIncrement(j["increment"].get<int>());
                if (j.contains("memoValue") && !j["memoValue"].empty()) this->setMemoValue(j["memoValue"].get<int>());
                if (j.contains("value") && !j["value"].empty()) this->setValue(j["value"].get<int>());
                if (j.contains("channel") && !j["channel"].empty()) this->setChannel(j["channel"].get<int>());
                if (j.contains("outputTOPIC") && !j["outputTOPIC"].empty()) this->setOutputTOPIC(j["outputTOPIC"].get<std::string>());
                if (j.contains("type") && !j["type"].empty()) this->setType(lightType_from_string(j["type"].get<std::string>()));
            }
    });

    //getConfiguration
    this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload) {
        if (topic == this->getGetTOPIC()) {
            json j = this->getConfigT()->getJsonConfig(this->getConfigT());
            j["value"] = this->getValue();
            this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
        }
    });

    /*this->getMqtt()->addNodeRedConnectedHandler([this]() {
        json j;
        j["name"] = this->getName();
        j["comment"] = this->getComment();
        j["value"] = this->getValue();
        j["memo_value"] = this->getMemoValue();
        this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
    });*/
}

void LIGHT::_onValueChange(int last_value, int value) {
  for(auto&& handler : this->valueChangeHandlers) {
    //std::thread(handler, last_value, value).detach();
    threadPool.push_task(handler, last_value, value);
  }    
}

void LIGHT::_onMemoValueChange(int last_value, int value) {
  for(auto&& handler : this->memoValueChangeHandlers) {
    //std::thread(handler, last_value, value).detach();
    threadPool.push_task(handler, last_value, value);
  }    
}


void LIGHT::setIncrement(int increment) {
    this->getConfigT()->increment = increment;
    this->saveConfig();
}

int LIGHT::getIncrement() {
    return this->getConfigT()->increment;
}

LIGHT_TYPE LIGHT::getType() {
    return this->getConfigT()->type;
}

void LIGHT::setType(LIGHT_TYPE type) {
    this->getConfigT()->type = type;
    this->saveConfig();
}

void LIGHT::setChannel(int channel) {
    this->getConfigT()->channel = channel;
    this->saveConfig();
}

int LIGHT::getChannel() {
    return this->getConfigT()->channel;
}


void LIGHT::setValue(int value) {
    //std::cout << "setValue(" << value <<") -> oldValue = " << this->value << std::endl << std::flush;
    if (value != this->value) {
        this->_onValueChange(this->value, value);
        this->value = value;
    }
}

int LIGHT::getValue() {
    return this->value;
}

void LIGHT::setMemoValue(int value) {
    this->_onMemoValueChange(this->getMemoValue(), value);
    this->getConfigT()->memo_value = value;
    this->saveConfig();
}

int LIGHT::getMemoValue() {
    return this->getConfigT()->memo_value;
}

std::string LIGHT::getOutputTOPIC() {
    return this->getConfigT()->output_TOPIC;
}

void LIGHT::setOutputTOPIC(std::string o) {
    this->getConfigT()->output_TOPIC = o;
    this->saveConfig();
}

void LIGHT::dimStart() {
    this->dimming = true;
}

void LIGHT::dimStop() {
    this->dimming = false;
    this->setMemoValue(this->getValue());
    this->updown = this->updown == -1 ? 1:-1;
}

void LIGHT::increment() {
    if (this->getType() == LIGHT_TYPE::DIMMABLE) {
        if (this->value + this->getIncrement() <= DIMMABLE_MAX_VALUE) {
            this->setValue(this->value + this->getIncrement());
        } else {
            this->setValue( DIMMABLE_MAX_VALUE );
        }
    }
}

void LIGHT::decrement() {
    if (this->getType() == LIGHT_TYPE::DIMMABLE) {
        if (this->value - this->getIncrement() >= 0) {
            this->setValue(this->value - this->getIncrement());
        } else {
            this->setValue(0);
        }
    }
}

void LIGHT::toggle() {
    if (this->getType() == LIGHT_TYPE::ONOFF) {
        //std::cout << this->value << ": " << (this->value == 0 ? 1 : 0) << std::endl << std::flush;
        this->setValue(this->value == 0 ? 1 : 0);
    } else if(this->getType() == LIGHT_TYPE::DIMMABLE) {
        //std::cout << this->value << ": " << (this->value != 0 ? 0 : this->getMemoValue()) << std::endl << std::flush;
        this->setValue(this->value !=0 ? 0 : this->getMemoValue());
    }
}

void LIGHT::process() {
    //process only for dim
    if (this->getType() == LIGHT_TYPE::DIMMABLE) {
        if (this->dimming) {
            if (this->value >= DIMMABLE_MAX_VALUE) this->updown = -1;
            if (this->value <= 0) this->updown = 1;
            switch (this->updown) {
                case 1 : this->increment(); break;
                case -1: this->decrement(); break;
            }
        }
    }
}

void LIGHT::_onMainThreadStart() {
    std::cout << "Starting "<< this->getName() << " thread..." << std::endl;
}

void LIGHT::_onMainThreadStopping() {
    std::cout << "Stopping "<< this->getName() << " thread..." << std::endl;    
}

void LIGHT::_onMainThreadStop() {    
    std::cout << this->getName() <<" thread stoped." << std::endl;
}


void LIGHT::addValueChangeHandler(valueChangeHandlersFunc function) {
    this->valueChangeHandlers.push_back(function);
}

void LIGHT::addMemoValueChangeHandler(valueChangeHandlersFunc function) {
    this->memoValueChangeHandlers.push_back(function);
}

LIGHT::~LIGHT () {

}

/* ============================================= */

class LIGHTS;


LIGHTS::LIGHTS (CONFIG *config, MyMqtt *myMqtt) {
    dmx_open();
    dmx_init();
    for (auto lightConf: config->getConfig()->lights->lights) {
        LIGHT *light = new LIGHT(config, lightConf, myMqtt);
        
        this->addLight(light);
    }
}

void LIGHTS::addLight(LIGHT *light) {
    this->lights.push_back(light);
}

LIGHT *LIGHTS::findByName(std::string name) {
    auto it = std::find_if(this->lights.begin(), this->lights.end(), [name](LIGHT *obj) {return obj->getName() == name;});
    return *it;
}

void LIGHTS::dump() {
  for(std::vector<LIGHT*>::iterator it = std::begin(this->lights); it != std::end(this->lights); ++it) {
    printf("Button %s, comment: %s\r\n",(*it)->getName(), (*it)->getComment()); fflush(stdout);
  }
}

void LIGHTS::startChildrenThreads() {
  std::cout << "Starting Lights children thread..." << std::endl;
  for(std::vector<LIGHT*>::iterator it = std::begin(this->lights); it != std::end(this->lights); ++it) {
    (*it)->start((*it)->getName().c_str());
  } 
}

void LIGHTS::stopChildrenThreads() {
  std::cout << "Stoping Lights children thread..." << std::endl;
  for(std::vector<LIGHT*>::iterator it = std::begin(this->lights); it != std::end(this->lights); ++it) {
    (*it)->stop();
  } 
}

void LIGHTS::joinChildrenThreads() {
  for(std::vector<LIGHT*>::iterator it = std::begin(this->lights); it != std::end(this->lights); ++it) {
    (*it)->getProcessThread()->join();
  } 
}

LIGHTS::~LIGHTS() {
  for(std::vector<LIGHT*>::iterator it = std::begin(this->lights); it != std::end(this->lights); ++it) {
    delete (*it);
  } 
    dmx_close();
}

