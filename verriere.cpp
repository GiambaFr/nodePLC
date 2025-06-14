#include <nlohmann/json.hpp>
#include <chrono>
#include <utility> 
#include <cmath>
#include <algorithm>

#include "commons.h"
#include "digital_output.h"
#include "verriere.h"
#include "config.h"

#include "threadPool.h"

using json = nlohmann::json;


class VERRIERE;

VERRIERE::VERRIERE(CONFIG *config, CONF::Verriere* verriereConf, MyMqtt *myMqtt, Digital_Output *outputUp, Digital_Output *outputDown) : withConfig(config, verriereConf), withMqtt(myMqtt), outputUp(outputUp), outputDown(outputDown) {

  this->setThreadSleepTimeMillis(200);
  this->current_position = this->getConfigT()->current_position;
  this->state = outputUp->getState() == STATE::ON ? VERRIERE_STATE::VERRIERE_MOVING_UP : outputDown->getState() == STATE::ON ? VERRIERE_STATE::VERRIERE_MOVING_DOWN : VERRIERE_STATE::VERRIERE_STOP;

  this->target_position = this->current_position;

  this->motion_start_time = std::chrono::steady_clock::now(); // Initialize
  this->last_mqtt_publish_time = std::chrono::steady_clock::now(); // Initialize

  this->init = false;

  this->outputUp->addStateChangeHandler([this](STATE oldState, STATE newState) {
    if (newState == STATE::ON) {
        this->setTargetPosition(100);
        this->setState(VERRIERE_STATE::VERRIERE_MOVING_UP);
    }
    if (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_UP && newState == STATE::OFF) {
        this->setState(VERRIERE_STATE::VERRIERE_STOP);
    }
  });
  this->outputDown->addStateChangeHandler([this](STATE oldState, STATE newState) {
    if (newState == STATE::ON) {
        this->setTargetPosition(0);
        this->setState(VERRIERE_STATE::VERRIERE_MOVING_DOWN);
    }
    if (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_DOWN && newState == STATE::OFF) {
        this->setState(VERRIERE_STATE::VERRIERE_STOP);
    }
  });


  this->addPositionChangeHandler([this](int oldPosition, int newPosition) {
    json j;
    j["event"] = "POSITION_CHANGE";
    j["state"] = verriereState_to_string(this->getState());
    j["old_position"] = oldPosition;
    j["position"] = newPosition;
    j["target"] = this->getTargetPosition();
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });

  this->addTargetChangeHandler([this](int oldTarget, int newTarget) {
    json j;
    j["event"] = "TARGET_CHANGE";
    j["state"] = verriereState_to_string(this->getState());
    j["position"] = this->getCurrentPosition();
    j["old_target"] = oldTarget;
    j["target"] = newTarget;
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });

  this->addStateChangeHandler([this](VERRIERE_STATE oldState, VERRIERE_STATE newState) {
    json j;
    j["event"] = "STATE_CHANGE";
    j["name"] = this->getName();
    j["comment"] = this->getComment();
    j["position"] = this->getCurrentPosition();
    j["target"] = this->getTargetPosition();
    j["old_state"] = verriereState_to_string(oldState);
    j["state"] = verriereState_to_string(newState);
    this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
  });



  this->addStateChangeHandler([this](VERRIERE_STATE oldState, VERRIERE_STATE newState) {
    //TODO COMMAND
    switch (newState) {
        case VERRIERE_STATE::VERRIERE_STOP:
            // Lorsque la verrière s'arrête, nous mettons à jour la position actuelle
                 this->setCurrentPosition(this->calculateCurrentPositionBasedOnTime()); 
                 this->target_position = this->current_position; // La cible devient la position actuelle
        break;
        case VERRIERE_STATE::VERRIERE_MOVING_DOWN:
            this->motion_start_time = std::chrono::steady_clock::now(); // Enregistrer le début du mouvement
        break;
        case VERRIERE_STATE::VERRIERE_MOVING_UP:
            this->motion_start_time = std::chrono::steady_clock::now(); // Enregistrer le début du mouvement
        break;
        case VERRIERE_STATE::VERRIERE_OPEN: // Cet état est atteint quand la position est 100
            this->outputUp->setState(STATE::OFF);
            this->outputDown->setState(STATE::OFF);
            this->setCurrentPosition(100);
            this->target_position = 100;
        break;
        case VERRIERE_STATE::VERRIERE_CLOSED: // Cet état est atteint quand la position est 0
            this->outputUp->setState(STATE::OFF);
            this->outputDown->setState(STATE::OFF);
            this->setCurrentPosition(0);
            this->target_position = 0;
            if (this->init == true) { // Si c'était une phase d'initialisation, réinitialiser le flag
                this->init = false;
            }
        break;
    }
  });

  this->getMqtt()->addMessageArrivedHandler([this](std::string topic, std::string payload){

    if(topic == this->getSetTOPIC()) {
        json j = json::parse(payload);
        if ( j.contains("action") && !j["action"].empty() ) {
          VERRIERE_ACTION action = verriereAction_from_string(j["action"].get<std::string>());
          // VERRIERE_STATE
          // VERRIERE_STOP, VERRIERE_MOVING_DOWN, VERRIERE_MOVING_UP, VERRIERE_OPEN, VERRIERE_CLOSED
          // VERRIERE_ACTION
          // VERRIERE_STOP, VERRIERE_DOWN, VERRIERE_UP, SET_PERCENTAGE
          switch (action) {
            case VERRIERE_ACTION::VERRIERE_STOP:
                if (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_UP || this->getState() == VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
                    this->outputUp->setState(STATE::OFF);
                    this->outputDown->setState(STATE::OFF); 
                    this->setState(VERRIERE_STATE::VERRIERE_STOP);
                }
            break;
            case VERRIERE_ACTION::VERRIERE_DOWN:
                if (this->getState() != VERRIERE_STATE::VERRIERE_MOVING_DOWN && this->getCurrentPosition() > 0) {
                    this->setTargetPosition(0); // Cible 0 pour une fermeture complète
                    this->outputDown->setState(STATE::ON); 
                    this->setState(VERRIERE_STATE::VERRIERE_MOVING_DOWN);
                }
            break;
            case VERRIERE_ACTION::VERRIERE_UP:
                if (this->getState() != VERRIERE_STATE::VERRIERE_MOVING_UP && this->getCurrentPosition() < 100) {
                    this->setTargetPosition(100); // Cible 100 pour une ouverture complète
                    this->outputUp->setState(STATE::ON); 
                    this->setState(VERRIERE_STATE::VERRIERE_MOVING_UP);
                }
            break;
            case VERRIERE_ACTION::SET_PERCENTAGE:
                if ( j.contains("percent") && !j["percent"].empty() ) {
                    int percent = j["percent"].get<int>();
                    if (percent < 0) percent = 0;
                    if (percent > 100) percent = 100;

                    if (percent != this->getCurrentPosition()) { // Seulement si la cible est différente de la position actuelle
                        this->setTargetPosition(percent);
                        if (percent > this->getCurrentPosition()) {
                            this->outputUp->setState(STATE::ON); 
                            this->setState(VERRIERE_STATE::VERRIERE_MOVING_UP);
                        } else if (percent < this->getCurrentPosition()) {
                            this->outputDown->setState(STATE::ON); 
                            this->setState(VERRIERE_STATE::VERRIERE_MOVING_DOWN);
                        }
                    } else { // Si la cible est la position actuelle, on s'arrête si on bougeait.
                        this->outputUp->setState(STATE::OFF);
                        this->outputDown->setState(STATE::OFF);
                        this->setState(VERRIERE_STATE::VERRIERE_STOP);
                    }
                }
            break;
            case VERRIERE_ACTION::VERRIERE_INIT:
                // L'initialisation doit être faite en déplaçant la verrière vers le bas jusqu'à ce qu'elle soit complètement fermée (position 0).
                // Cela réinitialise la position de référence.
                if (this->getState() != VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
                    this->init = true; // Active le mode initialisation
                    this->setTargetPosition(0); // La cible de l'initialisation est 0
                    this->outputDown->setState(STATE::ON); 
                    this->setState(VERRIERE_STATE::VERRIERE_MOVING_DOWN);
                }
            break;
          }
        }
    }
  });

}

void VERRIERE::_onPositionChange(int oldPosition, int newPosition) {
    for(auto&& handler : this->positionChangeHandlers) {
        threadPool.push_task(handler, oldPosition, newPosition);
    }
}

void VERRIERE::addPositionChangeHandler(positionChangeHandlersFunc function) {
  this->positionChangeHandlers.push_back(function);
}

void VERRIERE::_onTargetChange(int oldTarget, int newTarget) {
    for(auto&& handler : this->positionChangeHandlers) {
        threadPool.push_task(handler, oldTarget, newTarget);
    }
}

void VERRIERE::addTargetChangeHandler(targetChangeHandlersFunc function) {
  this->targetChangeHandlers.push_back(function);
}

int VERRIERE::getOpenDurationMs() {
    return this->getConfigT()->open_duration_ms;
}

void VERRIERE::setOpenDurationMs(int duration) {
    this->getConfigT()->open_duration_ms = duration;
    this->saveConfig();
}

int VERRIERE::getCloseDurationMs() {
    return this->getConfigT()->close_duration_ms;
}

void VERRIERE::setCloseDurationMs(int duration) {
    this->getConfigT()->close_duration_ms = duration;
    this->saveConfig();
}

int VERRIERE::getOpenSlowdownDurationMs() {
    return this->getConfigT()->open_slowdown_duration_ms;
}

void VERRIERE::setOpenSlowdownDurationMs(int duration) {
    this->getConfigT()->open_slowdown_duration_ms = duration;
    this->saveConfig();
}

int VERRIERE::getCloseSlowdownDurationMs() {
    return this->getConfigT()->close_slowdown_duration_ms;
}

void VERRIERE::setCloseSlowdownDurationMs(int duration) {
    this->getConfigT()->close_slowdown_duration_ms = duration;
    this->saveConfig();
}

int VERRIERE::getCurrentPosition() {
    return this->current_position; // Retourne la variable membre, pas la config directement pour une mise à jour rapide
    //return this->getConfigT()->current_position;
}

void VERRIERE::setCurrentPosition(int position) {
    if (this->current_position != position) {
        int oldPosition = this->current_position;
        this->current_position = position;
        this->getConfigT()->current_position = position; // Mettre à jour la config pour la persistance
        this->saveConfig(); // Sauvegarder la config après mise à jour
        this->_onPositionChange(oldPosition, position);
    }
}

int VERRIERE::getTargetPosition() {
    return this->target_position;
}

void VERRIERE::setTargetPosition(int position) {
    if (this->target_position != position) {
        int oldTarget = this->target_position;
        this->target_position = position;
        this->_onTargetChange(oldTarget, position);
        // Ne pas sauvegarder la target_position dans la config car c'est une cible temporaire
        // et non la position actuelle persistante.
        // this->motion_start_time est réglé lors du changement d'état (MOVING_UP/DOWN)
    }
}


int VERRIERE::timeToPercent(int totalTime, long timeMs) {
    return std::round(timeMs / totalTime * 100);
}

long VERRIERE::percentToTime(int totalTime, int percent) {
    return std::round(totalTime * percent / 100);
}

// --- VERRIERE::calculateCurrentPositionBasedOnTime() implementation ---
int VERRIERE::calculateCurrentPositionBasedOnTime() {
    long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->motion_start_time).count();
    int calculated_position = this->getCurrentPosition(); // Start with the last known position

    if (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_UP) {
        long totalOpenTime = this->getOpenDurationMs();
        long slowdownOpenTime = this->getOpenSlowdownDurationMs();
        long fullSpeedOpenTime = totalOpenTime - slowdownOpenTime;

        if (totalOpenTime == 0) return 100; // If duration is zero, assume instantly open

        double speed_full = 100.0 / totalOpenTime; // Percentage per millisecond for full range

        if (elapsed_ms <= fullSpeedOpenTime) {
            // Full speed phase
            calculated_position = this->current_position + std::round(speed_full * elapsed_ms);
        } else if (elapsed_ms > fullSpeedOpenTime && elapsed_ms <= totalOpenTime) {
            // Slowdown phase
            long slowdownElapsed = elapsed_ms - fullSpeedOpenTime;
            // Linear slowdown from full speed to 0 over slowdownOpenTime
            // The position covered during slowdown is proportional to the remaining time in slowdown.
            // Simplified: treat the slowdown as still contributing to the overall percentage.
            // A more complex model might integrate a speed curve.
            calculated_position = this->current_position + std::round(speed_full * elapsed_ms); 
        } else {
            // Beyond total opening time, assume fully open
            calculated_position = 100;
        }
        calculated_position = std::min(100, calculated_position); // Cap at 100
        
        // If the target is not 100, we should stop when the target is reached
        // The issue is that the motion_start_time and this calculation are for full travel.
        // For partial travel, we need to consider the distance to travel.
        if (this->target_position != 100) {
            // Recalculate based on current position and target, and duration for that specific travel.
            // This is a simplification; a more robust solution might involve calculating a dynamic 'speed'
            // for partial movements. For now, we estimate based on full travel time.
            double percentage_to_move = static_cast<double>(this->target_position - this->current_position);
            long estimated_duration_for_target = percentToTime(totalOpenTime, static_cast<int>(percentage_to_move));
            
            if (elapsed_ms >= estimated_duration_for_target) {
                calculated_position = this->target_position;
                this->outputUp->setState(STATE::OFF); // Stop the motor
                this->setState(VERRIERE_STATE::VERRIERE_STOP);
            }
        }

    } else if (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
        long totalCloseTime = this->getCloseDurationMs();
        long slowdownCloseTime = this->getCloseSlowdownDurationMs();
        long fullSpeedCloseTime = totalCloseTime - slowdownCloseTime;

        if (totalCloseTime == 0) return 0; // If duration is zero, assume instantly closed

        double speed_full = 100.0 / totalCloseTime; // Percentage per millisecond for full range

        if (elapsed_ms <= fullSpeedCloseTime) {
            // Full speed phase
            calculated_position = this->current_position - std::round(speed_full * elapsed_ms);
        } else if (elapsed_ms > fullSpeedCloseTime && elapsed_ms <= totalCloseTime) {
            // Slowdown phase
            long slowdownElapsed = elapsed_ms - fullSpeedCloseTime;
            calculated_position = this->current_position - std::round(speed_full * elapsed_ms);
        } else {
            // Beyond total closing time, assume fully closed
            calculated_position = 0;
        }
        calculated_position = std::max(0, calculated_position); // Cap at 0

        // If the target is not 0, we should stop when the target is reached
        if (this->target_position != 0) {
            double percentage_to_move = static_cast<double>(this->current_position - this->target_position);
            long estimated_duration_for_target = percentToTime(totalCloseTime, static_cast<int>(percentage_to_move));
            
            if (elapsed_ms >= estimated_duration_for_target) {
                calculated_position = this->target_position;
                this->outputDown->setState(STATE::OFF); // Stop the motor
                this->setState(VERRIERE_STATE::VERRIERE_STOP);
            }
        }
    }
    // For VERRIERE_STOP, VERRIERE_OPEN, VERRIERE_CLOSED states, the position is already stable
    // so no calculation is needed here. The position should be set to current_position itself.

    return calculated_position;
}


// --- VERRIERE::process() implementation ---
void VERRIERE::process() {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    // Update position if moving
    if (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_UP || this->getState() == VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
        int new_calculated_position = this->calculateCurrentPositionBasedOnTime();
        if (new_calculated_position != this->current_position) {
            this->current_position = (new_calculated_position);
        }

        // Check if target position is reached during continuous movement
        if (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_UP && this->getCurrentPosition() >= this->getTargetPosition()) {
            this->outputUp->setState(STATE::OFF);
            this->setState(VERRIERE_STATE::VERRIERE_STOP);
            this->current_position = (this->getTargetPosition()); // Ensure it's exactly at target
        } else if (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_DOWN && this->getCurrentPosition() <= this->getTargetPosition()) {
            this->outputDown->setState(STATE::OFF);
            this->setState(VERRIERE_STATE::VERRIERE_STOP);
            this->current_position = (this->getTargetPosition()); // Ensure it's exactly at target
        }

        // Special handling for full open/close if exact position hit, even during slowdown
        if (this->getCurrentPosition() == 100 && this->getState() == VERRIERE_STATE::VERRIERE_MOVING_UP) {
            this->setState(VERRIERE_STATE::VERRIERE_OPEN);
        } else if (this->getCurrentPosition() == 0 && this->getState() == VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
            this->setState(VERRIERE_STATE::VERRIERE_CLOSED);
        }
    }
        // Publication MQTT périodique pour les mouvements
        long elapsed_since_last_publish = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->last_mqtt_publish_time).count();
        if (elapsed_since_last_publish >= 1000) { // Publier toutes les secondes pendant le mouvement
            this->last_mqtt_publish_time = now;
            this->setCurrentPosition(this->current_position);
        }
}

void VERRIERE::_onMainThreadStart() {
    std::cout << "Starting "<< this->getName() << " thread..." << std::endl;
}


void VERRIERE::_onMainThreadStopping() {
    std::cout << "Stopping "<< this->getName() << " thread..." << std::endl;  
}

void VERRIERE::_onMainThreadStop() {
    std::cout << this->getName() <<" thread stoped." << std::endl;
}


VERRIERE::~VERRIERE() {
    this->outputUp->setState(STATE::OFF);
    this->outputDown->setState(STATE::OFF); // Assurez-vous que les deux sont éteintes à la destruction
}

/* =================================================================*/
class VERRIERES;

VERRIERES::VERRIERES(CONFIG* config, MyMqtt* myMqtt, Digital_Outputs *DOs) {
    for (auto verriereConf: config->getConfig()->verrieres->verrieres) {
      VERRIERE *V = new VERRIERE(config, verriereConf, myMqtt, DOs->findByName(verriereConf->up_DoName), DOs->findByName(verriereConf->down_DoName));
      this->addVerriere(V);
      std::cout << "Added Verriere name: " << V->getName() << " comment: " << V->getComment() << std::endl;
    }
}


void VERRIERES::addVerriere(VERRIERE* V) {
    this->verrieres.push_back(V);
}

VERRIERE *VERRIERES::findByName(std::string name) {
    auto it = std::find_if(this->verrieres.begin(), this->verrieres.end(), [name](VERRIERE *obj) {return obj->getName() == name;});
    // Consider adding a check here for iterator validity before dereferencing (*it)
    if (it != this->verrieres.end()) {
        return *it;
    }
    return nullptr; // Return nullptr if not found to avoid dereferencing an invalid iterator;
}

void VERRIERES::dump() {

}

void VERRIERES::startChildrenThreads() {
  std::cout << "Starting Verrieres children thread..." << std::endl;
  for(std::vector<VERRIERE*>::iterator it = std::begin(this->verrieres); it != std::end(this->verrieres); ++it) {
    (*it)->start((*it)->getName().c_str());
  } 
}

void VERRIERES::stopChildrenThreads() {
  std::cout << "Stoping Verrieres thread..." << std::endl;
  for(std::vector<VERRIERE*>::iterator it = std::begin(this->verrieres); it != std::end(this->verrieres); ++it) {
    (*it)->stop();
  } 
}

void VERRIERES::joinChildrenThreads() {
    for(std::vector<VERRIERE*>::iterator it = std::begin(this->verrieres); it != std::end(this->verrieres); ++it) {
    (*it)->getProcessThread()->join();
  } 
}

VERRIERES::~VERRIERES() {
  for(std::vector<VERRIERE*>::iterator it = std::begin(this->verrieres); it != std::end(this->verrieres); ++it) {
    (*it)->stop();
    delete (*it);
    (*it) = nullptr;
  } 
}
