#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <wiringPi.h>
#include <cstdlib>
#include <regex> 

#include "commons.h"
#include "digital_input.h"

#include "bt.h"

using json = nlohmann::json;



class Digital_Input;

Digital_Input::Digital_Input(CONFIG *config, CONF::Input* inputConf, MyMqtt *myMqtt) : withConfig(config, inputConf), withMqtt(myMqtt) {

    this->setThreadSleepTimeMillis(50);

    pinMode(this->getPin(), INPUT);
    pullUpDnControl(this->getPin(), PUD_UP);


    if (this->isButton()) {
      this->button = new BT();
      this->button->dcGap = this->getConfigT()->button.DC_GAP;
      this->button->holdTime = this->getConfigT()->button.HOLD_TIME;
      this->button->addActionHandler([this](BUTTON_ACTION btAction, unsigned int clickCount) {
        json j;
        j["action"] = btAction_to_string(btAction);
        j["clickCount"] = clickCount;
        j["name"] = this->getName();
        j["comment"] = this->getComment();
        this->getMqtt()->publish(this->getButtonDispatchTOPIC(), j, false);
      });
    }


    this->addStateChangeHandler([this](STATE oldState, STATE newState) {
      json j;
      j["event"] = "STATE_CHANGE";
      j["state"] = state_to_string(newState);
      j["name"] = this->getName();
      j["comment"] = this->getComment();
      this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
    });

    this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload){
      if(topic == this->getSetTOPIC()) {
          json j = json::parse(payload);
          if (j.contains("state") && !j["state"].empty())
            this->setState( state_from_string(j["state"].get<std::string>()) );
          this->setBaseParams(j);
          if (j.contains("pin") && !j["pin"].empty())
            this->setPin(j["pin"].get<int>());
      }

    });

    //getConfiguration
    this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload) {
      if(topic == this->getGetTOPIC()) {

        json j = this->getConfigT()->getJsonConfig(this->getConfigT());
        
        j["state"] = state_to_string(this->getState());
        this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
      }
    });
}


int Digital_Input::getPin() {
  return this->getConfigT()->pin;
}

void Digital_Input::setPin(int pin) {
  this->getConfigT()->pin = pin;
  this->saveConfig();  
}


STATE Digital_Input::_read() {
  int value = !digitalRead(this->getPin());
  return value == HIGH ? STATE::ON : STATE::OFF;
}

void Digital_Input::_onMainThreadStart() {
  std::cout << "Starting Input " << this->getName() << " thread..." << std::endl;
}

void Digital_Input::_onMainThreadStopping() {
  std::cout << "Stoping Input " << this->getName() << " thread..." << std::endl;  
}

void Digital_Input::_onMainThreadStop() {
  std::cout << "Input " << this->getName() << " stoped." << std::endl;  
}


void Digital_Input::process() {
    STATE newState = this->_read();
    this->setState(newState);
    if (this->button != nullptr) this->button->process(newState);
}

bool Digital_Input::isButton() {
  return this->getConfigT()->button.isButton;
}

std::string Digital_Input::getButtonDispatchTOPIC() {
  return this->getConfigT()->button.buttonDispatch_TOPIC;
}


Digital_Input::~Digital_Input() {
  if (this->button != nullptr)
    delete (this->button);
}

/* =================================================================*/
class Digital_Inputs;

Digital_Inputs::Digital_Inputs(CONFIG *config, MyMqtt *myMqtt) {
    wiringPiSetupGpio();
    for (auto inputConf: config->getConfig()->inputs->inputs) {
      Digital_Input *DI = new Digital_Input(config, inputConf, myMqtt);
      this->addInput(DI);
      std::cout << "Added Input name: " << DI->getName() << " comment: " << DI->getComment() << std::endl;
    }
}


void Digital_Inputs::addInput(Digital_Input *input) {
    this->inputs.push_back(input);
}

Digital_Input *Digital_Inputs::findByName(std::string name) {
    auto it = std::find_if(this->inputs.begin(), this->inputs.end(), [name](Digital_Input *obj) {return obj->getName() == name;});
    return *it;
}


void Digital_Inputs::dump() {
  for(std::vector<Digital_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    printf("Input %s, pin:%d, comment: %s\r\n",(*it)->getName(), (*it)->getPin(), (*it)->getComment()); fflush(stdout);
  }
}


void Digital_Inputs::startChildrenThreads() {
  std::cout << "Starting Digital_Input children thread..." << std::endl;
  for(std::vector<Digital_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    (*it)->start();
  } 
}

void Digital_Inputs::stopChildrenThreads() {
  std::cout << "Stoping Digital_Input children thread..." << std::endl;
  for(std::vector<Digital_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    (*it)->stop();
  } 
}

void Digital_Inputs::joinChildrenThreads() {
  for(std::vector<Digital_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    (*it)->getProcessThread()->join();
  } 
}


Digital_Inputs::~Digital_Inputs() {
  for(std::vector<Digital_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    delete (*it);
  }    
}