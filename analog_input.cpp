#include <fstream>
#include <owcapi.h>
#include <cstdlib>

#include <string>
#include <iomanip>
#include <thread>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <nlohmann/json.hpp>

//#include "wiringPiI2C.h"
#include "wiringPi.h"

#include <mcp3422.h>
#include "analog_input.h"

#include "threadPool.h"

using json = nlohmann::json;
using std::string;

class Analog_Input;

Analog_Input::Analog_Input(CONFIG *config, CONF::Analog_Input *aInputConf, MyMqtt *myMqtt, int fd, double lsb, std::mutex *i2cMutex) : withConfig(config, aInputConf), withMqtt(myMqtt), fd(fd), lsb(lsb), i2cMutex(i2cMutex) {

  this->setThreadSleepTimeMillis(1000);


  this->addVoltageChangeHandler([this](double oldValue, double newValue) {
    json j;
    j["event"] = "VOLTAGE_CHANGE";
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    j["last_voltage"] = oldValue;
    j["voltage"] = newValue;
    j["value"] = newValue;
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });

  //getConfiguration
  this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload) {
    if (topic == this->getGetTOPIC()) {
      json j = this->getConfigT()->getJsonConfig(this->getConfigT());  
      j["event"] = "REQUEST";
      j["voltage"] = this->getValue();
      j["value"] = this->getValue();
      this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
    }
  });

  this->value = 0;

}

double Analog_Input::_read() {
    std::lock_guard<std::mutex> lock(*i2cMutex);
    double value;
    value = analogRead (MCP3422_PIN_BASE + this->getChannel()) - 16;
    //std::cout << std::to_string(value) << std::endl;
    value = value * this->lsb / 1000 * this->getK() / 1000;
    //std::cout << std::to_string(value) << std::endl;
    return value;
}

void Analog_Input::_onVoltageChange(double oldValue, double newValue) {
  for(auto&& handler : this->voltageChangeHandlers) {
    //std::thread t(handler, oldValue, newValue);
    //t.detach();
    threadPool.push_task(handler, oldValue, newValue);
  }
}

int Analog_Input::getChannel() {
  return this->getConfigT()->channel;
}

double Analog_Input::getK() {
  return this->getConfigT()->K;
}

double Analog_Input::getValue() {
    return this->value;
}


void Analog_Input::process() {
  //std::cout << "reading " << this->getName() << "... " << std::endl << std::flush;
    double newValue = this->_read();
    newValue = std::round(newValue*100000)/100000;
    if (newValue != this->value) {
        this->_onVoltageChange(this->value, newValue);
        this->value = newValue;
    }
    //std::cout << this->getName() << " = " << newValue << std::endl << std::flush;
}

void Analog_Input::_onMainThreadStart() {
    std::cout << "Starting "<< this->getName() << " thread..." << std::endl;
}

void Analog_Input::_onMainThreadStopping() {
    std::cout << "Stopping "<< this->getName() << " thread..." << std::endl;    
}

void Analog_Input::_onMainThreadStop() {    
    std::cout << this->getName() <<" thread stoped." << std::endl;
}


void Analog_Input::addVoltageChangeHandler(voltageChangeHandlersFunc function) {
    this->voltageChangeHandlers.push_back(function);
}

Analog_Input::~Analog_Input() {
    
}

/* ================================================== */
class Analog_Inputs;

Analog_Inputs::Analog_Inputs(CONFIG *config, MyMqtt *myMqtt) {


    //this->fd = wiringPiI2CSetup(0x68);
    mcp3422Setup (MCP3422_PIN_BASE, config->getConfig()->AInputs->MCP3422_Addr, config->getConfig()->AInputs->sampleRate, config->getConfig()->AInputs->gain) ;

    for (auto inputConf: config->getConfig()->AInputs->inputs) {
      Analog_Input *input = new Analog_Input(config, inputConf, myMqtt, this->fd, config->getConfig()->AInputs->lsb, &this->i2cMutex);

      this->addChannel(input);
      std::cout << "Added Analog_Input name:" << input->getName() << " comment:" << input->getComment() << " LSB:" << config->getConfig()->AInputs->lsb << " GAIN:" << config->getConfig()->AInputs->gain << std::endl;
    }
}

void Analog_Inputs::addChannel(Analog_Input *input) {
    this->inputs.push_back(input);
}

Analog_Input *Analog_Inputs::findByName(std::string name) {
    auto it = std::find_if(this->inputs.begin(), this->inputs.end(), [name](Analog_Input *obj) {return obj->getName() == name;});
    return *it;
}


void Analog_Inputs::dump() {
  for(std::vector<Analog_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    printf("Input %s, channel:%d, comment: %s\r\n",(*it)->getName(), (*it)->getChannel(), (*it)->getComment()); fflush(stdout);
  }
}

void Analog_Inputs::startChildrenThreads() {
  std::cout << "Starting Analog_Inputs children thread..." << std::endl;
  for(std::vector<Analog_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    (*it)->start((*it)->getName().c_str());
  } 
}

void Analog_Inputs::stopChildrenThreads() {
  std::cout << "Stoping Analog_Inputs children thread..." << std::endl;
  for(std::vector<Analog_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    (*it)->stop();
  } 
}

void Analog_Inputs::joinChildrenThreads() {
  for(std::vector<Analog_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    (*it)->getProcessThread()->join();
  } 
}

Analog_Inputs::~Analog_Inputs() {
  for(std::vector<Analog_Input*>::iterator it = std::begin(this->inputs); it != std::end(this->inputs); ++it) {
    delete (*it);
  } 
  
}