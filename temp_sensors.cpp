#include <fstream>
#include <owcapi.h>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <thread>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "temp_sensors.h"

#include "threadPool.h"

using json = nlohmann::json;
using std::string;

Temp_Sensor::Temp_Sensor(CONFIG *config, CONF::OW_Sensor* sensorConf, MyMqtt *myMqtt) : withConfig(config, sensorConf), withMqtt(myMqtt) {

  this->setThreadSleepTimeMillis(200);

  std::string reversedAddress = this->getAddress();
  reverse(reversedAddress.begin(),reversedAddress.end());
  this->sensorPath = "/sys/bus/w1/devices/28-" + reversedAddress + "/temperature";
  /*this->addTemperatureChangeHandler([this](float oldValue, float newValue) {
    std::cout << this->getName() << " = " << oldValue << " " << newValue << std::endl;
  });*/

  this->addTemperatureChangeHandler([this](float oldValue, float newValue) {
    json j;
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    j["last_temperature"] = oldValue;
    j["temperature"] = newValue;
    if (newValue > this->getAlarm()) 
      j["alarm"] = true; 
    else
      j["alarm"] = false;
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });

  this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload){
    if (topic == this->getSetTOPIC()) {
      json params;
      params = json::parse(payload);
      //parameters
      this->setBaseParams(params);
      if (params.contains("address") && !params["address"].empty()) {
          this->setAddress(params["address"].get<std::string>());
      }
      if (params.contains("min") && !params["min"].empty()) {
          this->setMin(params["min"].get<float>());
      }
      if (params.contains("max") && !params["max"].empty()) {
          this->setMax(params["max"].get<float>());
      }
      if (params.contains("alarm") && !params["alarm"].empty()) {
          this->setAlarm(params["alarm"].get<float>());
      }
    }
  });

  //getConfiguration
  this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload) {
    if (topic == this->getGetTOPIC()) {
      json j = this->getConfigT()->getJsonConfig(this->getConfigT());
      j["temperature"] = this->getValue();
      this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
    }
  });

  /*this->getMqtt()->addNodeRedConnectedHandler([this]() {
    json j;
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    j["temperature"] = this->getValue();
    if (this->getValue() > this->getAlarm()) 
      j["alarm"] = true; 
    else
      j["alarm"] = false;
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });*/

  this->value = this->_read();

}

float Temp_Sensor::_read() {
 // float value = this->value;
  
    char * get_buffer;
    std::size_t get_length;
    ssize_t get_return;
    float value;
    char path[50];
    size_t len = sprintf(path, TEMP_SENSOR_PATHTEMPLATE, this->getAddress().c_str());
    path[len] = '\0';
    //std::cout << "Try to read sensor " << this->getName() << std::endl;
    //get_return = OW_get(path,&get_buffer,&get_length);
    OW_get(path,&get_buffer,&get_length);
    if (get_buffer) {
        value = atof(get_buffer);
    } else {
        value = BAD_VALUE;
       // std::cout << "ERROR (" << errno << ") reading " << this->getName() << ": " << path << " ("<< strerror(errno) <<")" << std::endl;
    }
    free(get_buffer);
  

   /* if (std::filesystem::exists(this->sensorPath)) {
      std::ifstream infile(this->sensorPath);

      std::string valueString;
      getline(infile, valueString);

      infile.close();
      value = atof(valueString.c_str()) / 1000;
    }*/
    return value;
}

std::string Temp_Sensor::getAddress() {
    return this->getConfigT()->address;
}

void Temp_Sensor::setAddress(std::string address) {
    this->getConfigT()->address = address;
    this->saveConfig();
}

float Temp_Sensor::getMin() {
    return this->getConfigT()->min;
}

void Temp_Sensor::setMin(float v) {
    this->getConfigT()->min = v;
    this->saveConfig();  
}

float Temp_Sensor::getMax() {
    return this->getConfigT()->max;  
}

void Temp_Sensor::setMax(float v) {
    this->getConfigT()->max = v;
    this->saveConfig();  
}

float Temp_Sensor::getAlarm() {
    return this->getConfigT()->alarm;    
}

void Temp_Sensor::setAlarm(float v) {
    this->getConfigT()->alarm = v;
    this->saveConfig();
}

void Temp_Sensor::_onTemperatureChange(float oldValue, float newValue) {
  for(auto&& handler : this->temperatureChangeHandlers) {
    //std::thread t(handler, oldValue, newValue);
    //t.detach();
    threadPool.push_task(handler, oldValue, newValue);
  }
}

float Temp_Sensor::getValue() {
    return this->value.load(); // Use atomic load
}


void Temp_Sensor::process() {
  //std::cout << "reading " << this->getName() << "... " << std::endl << std::flush;
    float newValue = this->_read();
    if (newValue != BAD_VALUE) {
       if (newValue != this->value.load()) {
        this->_onTemperatureChange(this->value.load(), newValue);
        this->value.store(newValue);
      }
    }
    //std::cout << this->getName() << " = " << newValue << std::endl << std::flush;
}

void Temp_Sensor::_onMainThreadStart() {
    std::cout << "Starting "<< this->getName() << " thread..." << std::endl;
}

void Temp_Sensor::_onMainThreadStopping() {
    std::cout << "Stopping "<< this->getName() << " thread..." << std::endl;    
}

void Temp_Sensor::_onMainThreadStop() {    
    std::cout << this->getName() <<" thread stoped." << std::endl;
}


void Temp_Sensor::addTemperatureChangeHandler(temperatureChangeHandlersFunc function) {
    this->temperatureChangeHandlers.push_back(function);
}

Temp_Sensor::~Temp_Sensor() {
    
}

/* ================================================== */
class Temp_Sensors;

Temp_Sensors::Temp_Sensors(CONFIG *config, MyMqtt *myMqtt) {

    int ret;
    char owParams[50];
    const char *owParamsTemplate = "%s:%d";
    sprintf(owParams, owParamsTemplate, config->getConfig()->ow->server.c_str(), config->getConfig()->ow->port);
    std::cout << "ow_init(" << owParams << ")" << std::endl;
    ret = OW_init(owParams);
    if (ret != 0) {
        std::cout << "Error starting OneWire" << std::endl;
        std::cout << "ERROR (" << errno << ")" << " ("<< strerror(errno) <<")" << std::endl;
        exit(EXIT_FAILURE);
    }


    for (auto sensorConf: config->getConfig()->ow->sensors) {
      Temp_Sensor *tempSensor = new Temp_Sensor(config, sensorConf, myMqtt);

      this->addTempSensor(tempSensor);
      std::cout << "Added Temp_Sensor address: " << tempSensor->getAddress() << " name: " << tempSensor->getName() << " comment: " << tempSensor->getComment() << std::endl;
    }
}



void Temp_Sensors::addTempSensor(Temp_Sensor *sensor) {
    this->sensors.push_back(sensor);
}

Temp_Sensor *Temp_Sensors::findByName(std::string name) {
    auto it = std::find_if(this->sensors.begin(), this->sensors.end(), [name](Temp_Sensor *obj) {return obj->getName() == name;});
    // Consider adding a check here for iterator validity before dereferencing (*it)
    if (it != this->sensors.end()) {
        return *it;
    }
    return nullptr; // Return nullptr if not found to avoid dereferencing an invalid iterator;
}


void Temp_Sensors::dump() {
  for(std::vector<Temp_Sensor*>::iterator it = std::begin(this->sensors); it != std::end(this->sensors); ++it) {
    printf("Sensor %s, address:%d, comment: %s\r\n",(*it)->getName(), (*it)->getAddress(), (*it)->getComment()); fflush(stdout);
  }
}

void Temp_Sensors::startChildrenThreads() {
  std::cout << "Starting Temp_Sensors children thread..." << std::endl;
  for(std::vector<Temp_Sensor*>::iterator it = std::begin(this->sensors); it != std::end(this->sensors); ++it) {
    (*it)->start();
  } 
}

void Temp_Sensors::stopChildrenThreads() {
  std::cout << "Stoping Temp_Sensors children thread..." << std::endl;
  for(std::vector<Temp_Sensor*>::iterator it = std::begin(this->sensors); it != std::end(this->sensors); ++it) {
    (*it)->stop();
  } 
}

/*void Temp_Sensors::joinChildrenThreads() {
  for(std::vector<Temp_Sensor*>::iterator it = std::begin(this->sensors); it != std::end(this->sensors); ++it) {
    (*it)->getProcessThread()->join();
  } 
}*/

Temp_Sensors::~Temp_Sensors() {
  for(std::vector<Temp_Sensor*>::iterator it = std::begin(this->sensors); it != std::end(this->sensors); ++it) {
    delete (*it);
  } 
  OW_finish();
}