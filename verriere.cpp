#include <nlohmann/json.hpp>

#include "commons.h"
#include "verriere.h"

#include "threadPool.h"

using json = nlohmann::json;


class VERRIERE;

VERRIERE::VERRIERE(CONFIG *config, CONF::Verriere* verriereConf, MyMqtt *myMqtt) : withConfig(config, verriereConf), withMqtt(myMqtt) {


}



void VERRIERE::process() {

}

void VERRIERE::_onMainThreadStart() {

}

void VERRIERE::_onMainThreadStopping() {

}

void VERRIERE::_onMainThreadStop() {

}


VERRIERE::~VERRIERE() {

}


