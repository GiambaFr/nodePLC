#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cmath>
#include <nlohmann/json.hpp>
#include <wiringPi.h>
#include <mcp23008.h>

#include "commons.h"
#include "digital_output.h"


using namespace std;
using json = nlohmann::json;

class Digital_Output;

Digital_Output::Digital_Output(CONFIG *config, CONF::Output *outputConf, MyMqtt *myMqtt, Digital_Outputs *DOs) : withConfig(config, outputConf), withMqtt(myMqtt), DOs(DOs) {
  this->setThreadSleepTimeMicro(10); //10ms


  pinMode(this->getPin(), OUTPUT);

  this->addStateChangeHandler([this](STATE oldState, STATE newState) {
    if (newState != STATE::DIM) {
      this->_write(newState);
    } 
  });

  this->addStateChangeHandler([this](STATE oldState, STATE newState) {
    json j;
    j["state"] = state_to_string(newState);
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    if (this->isDimmable()) {
      j["value"] = std::to_string(this->getDimValue());
      j["dimCycleTime"] = std::to_string(this->getDimCycleTime());
    } 
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });


  this->addDimValueChangeHandler([this](int oldDimValue, int newDimValue) {
    json j;
    j["state"] = state_to_string(this->getState());
    j["value"] = std::to_string(newDimValue);
    j["dimCycleTime"] = std::to_string(this->getDimCycleTime());
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });

  this->addDimCycleTimeChangeHandler([this](int oldDimCycleTime, int newDimCycleTime) {
    json j;
    j["state"] = state_to_string(this->getState());
    j["value"] = std::to_string(this->getDimValue());
    j["dimCycleTime"] = std::to_string(newDimCycleTime);
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });


  for(auto lock: this->getConfigT()->locks) {
    this->addLock(lock);
  }

  this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload){

    if(topic == this->getSetTOPIC()) {
        json j = json::parse(payload);
        
        // forced
        if (j.contains("forced") && !j["forced"].empty() && j.contains("forcedValue") && !j["forcedValue"].empty()) {
          this->setForced(j["forced"].get<bool>());
          this->setForcedValue(state_from_string(j["forcedValue"].get<std::string>()));
          this->setState(this->getForcedValue());
        }

        // retain
        if (j.contains("retain") && !j["retain"].empty())
          this->setRetain(j["retain"].get<bool>());  

        //state
        if ( j.contains("state") && !j["state"].empty() && !this->isForced() ) {
          STATE newState = state_from_string(j["state"].get<std::string>());
          if (this->hasLock() && ( newState == STATE::ON || newState == STATE::DIM ) ) {
            for (std::string lock: this->getLocks()) {
              Digital_Output * DO = this->DOs->findByName(lock);
              if (DO->getState() == STATE::ON || DO->getState() == STATE::DIM)
                DO->setState(STATE::OFF);
            }
          }
          if (newState == STATE::DIM && !this->isDimmable())
            newState = STATE::ON;
          this->setState( newState );
          if (this->isRetain()) this->setRetainValue( newState );
        }

        //toggle
        if ( j.contains("toggle") && !this->isForced() ) this->toggle();

        //dim value
        if ( j.contains("state") && j["state"].get<string>() == state_to_string(STATE::DIM) && !this->isForced() && this->isDimmable()) {
            if (j.contains("value") && !j["value"].empty()) {
                int value = j["value"].get<int>();
                if ( value == 0 ) {
                  this->setState(STATE::OFF);
                } else {
                  this->setDimValue(value);
                  this->setState(STATE::DIM);
                }
                  
            }
        }
        //dimCycleTime
        if (j.contains("dimCycleTime") && !j["dimCycleTime"].empty()) {
          int dimCycleTime = j["value"].get<int>();
          if ( dimCycleTime >= 100 ) {
            this->setDimCycleTime(dimCycleTime);
          }
        }

        //params
        if ( j.contains("pin") && !j["pin"].empty() ) this->setPin(j["pin"].get<int>());


    }
  });

  //getConfiguration
  this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload) {
      if (topic == this->getGetTOPIC()) {
        json j = this->getConfigT()->getJsonConfig(this->getConfigT());
        j["state"] = state_to_string(this->getState());
        if (this->isDimmable()) {
          j["value"] = std::to_string(this->getDimValue());
          j["dimCycleTime"] = std::to_string(this->getDimCycleTime());
        } 
        this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
      }
  });

  this->setState(STATE::OFF);

  if (this->isRetain() && !this->isForced())
    this->setState(this->getRetainValue());

  if (this->isForced())
    this->setState(this->getForcedValue());

  /*this->getMqtt()->addNodeRedConnectedHandler([this]() {
      json j;
      j["name"] = this->getName();
      j["comment"] = this->getComment();
      j["state"] = state_to_string(this->getState());
      this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });*/

}

int Digital_Output::getPin() {
  return MCP23008_PIN_BASE + this->getConfigT()->pin;
}

void Digital_Output::setPin(int pin) {
  this->getConfigT()->pin = pin;
  this->saveConfig();
}


void Digital_Output::setForced(bool forced) {
  this->getConfigT()->forced = forced;
  this->saveConfig();
}

bool Digital_Output::isForced() {
  return this->getConfigT()->forced;
}

STATE Digital_Output::getForcedValue() {
  return state_from_string(this->getConfigT()->forcedValue);
}

void Digital_Output::setForcedValue(STATE value) {
  this->getConfigT()->forcedValue = state_to_string(value);
  this->saveConfig();
}

void Digital_Output::setRetain(bool retain) {
  this->getConfigT()->retain = retain;
  this->saveConfig();
}

bool Digital_Output::isRetain() {
  return this->getConfigT()->retain;
}

STATE Digital_Output::getRetainValue() {
  return state_from_string(this->getConfigT()->retainValue);
}

void Digital_Output::setRetainValue(STATE value) {
  this->getConfigT()->retainValue = state_to_string(value);
  this->saveConfig();
}

bool Digital_Output::isDimmable() {
  return this->getConfigT()->dimmable;
}


void Digital_Output::addLock(std::string lock) {
  this->locks.push_back(lock);
}

bool Digital_Output::hasLock() {
  return this->locks.size() > 0;
}

std::vector<std::string> Digital_Output::getLocks() {
  return this->locks;
}

STATE Digital_Output::_read() {
    return digitalRead(this->getPin()) == HIGH ? STATE::ON : STATE::OFF;
}

void Digital_Output::_write(STATE state) {
  if (state == STATE::ON)
    digitalWrite(this->getPin(), HIGH);
  else
    digitalWrite(this->getPin(), LOW);
}


void Digital_Output::toggle() {
  this->setState(this->getState() == (STATE::OFF) ? STATE::ON : STATE::OFF);
}


void Digital_Output::process() {
  if (this->isDimmable() && this->getState() == STATE::DIM) {
    int onTime = std::lround( (static_cast<float>(this->getDimValue()) / 100) * this->getDimCycleTime());
    digitalWrite(this->getPin(), HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(onTime));
    digitalWrite(this->getPin(), LOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(this->getDimCycleTime() - onTime));
  }
}

void Digital_Output::_onMainThreadStart() {
  std::cout << "Starting Output " << this->getName() << " thread..." << std::endl;
}

void Digital_Output::_onMainThreadStopping() {
  std::cout << "Stoping Output " << this->getName() << " thread..." << std::endl;  
}

void Digital_Output::_onMainThreadStop() {
  std::cout << "Output " << this->getName() << " stoped." << std::endl;  
}



Digital_Output::~Digital_Output() {
  
}

/* =================================================================*/
class Digital_Outputs;

Digital_Outputs::Digital_Outputs(CONFIG *config, MyMqtt *myMqtt) {
    wiringPiSetupGpio();
    mcp23008Setup (MCP23008_PIN_BASE, MCP23008_ADDRESS);
    mcp23008Setup (MCP23008_PIN_BASE+8, MCP23008_EXT_ADDRESS);
    for (auto outputConf: config->getConfig()->outputs->outputs) {
      Digital_Output *DO = new Digital_Output(config, outputConf, myMqtt, this);
      this->addOutput(DO);
      std::cout << "Added Output name: " << DO->getName() << " forced: " << BoolToString(DO->isForced())  << " forcedValue:" << state_to_string(DO->getForcedValue()) << " retain: " << BoolToString(DO->isRetain())  << " retainValue: " << state_to_string(DO->getRetainValue()) << " comment: " << DO->getComment() << std::endl;
    }
}


void Digital_Outputs::addOutput(Digital_Output *output) {
    this->outputs.push_back(output);
}

Digital_Output *Digital_Outputs::findByName(std::string name) {
    auto it = std::find_if(this->outputs.begin(), this->outputs.end(), [name](Digital_Output *obj) {return obj->getName() == name;});
    return *it;
}


void Digital_Outputs::dump() {
  for(std::vector<Digital_Output*>::iterator it = std::begin(this->outputs); it != std::end(this->outputs); ++it) {
    printf("Input %s, pin:%d, comment: %s\r\n",(*it)->getName(), (*it)->getPin(), (*it)->getComment()); fflush(stdout);
  }
}



void Digital_Outputs::startChildrenThreads() {
  std::cout << "Starting Digital_Outputs (DIMMABLE) children thread..." << std::endl;
  for(std::vector<Digital_Output*>::iterator it = std::begin(this->outputs); it != std::end(this->outputs); ++it) {
    if ( (*it)->isDimmable() )
      (*it)->start();
  } 
}

void Digital_Outputs::stopChildrenThreads() {
  std::cout << "Stoping Digital_Outputs children thread..." << std::endl;
  for(std::vector<Digital_Output*>::iterator it = std::begin(this->outputs); it != std::end(this->outputs); ++it) {
    (*it)->stop();
  } 
}

void Digital_Outputs::joinChildrenThreads() {
  for(std::vector<Digital_Output*>::iterator it = std::begin(this->outputs); it != std::end(this->outputs); ++it) {
    (*it)->getProcessThread()->join();
  } 
}


Digital_Outputs::~Digital_Outputs() {
  for(std::vector<Digital_Output*>::iterator it = std::begin(this->outputs); it != std::end(this->outputs); ++it) {
    delete (*it);
  }    
}