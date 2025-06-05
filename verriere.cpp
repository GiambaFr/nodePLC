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

  this->current_position = this->getConfigT()->current_position;
  this->state = outputUp->getState() == STATE::ON ? VERRIERE_STATE::VERRIERE_MOVING_UP : outputDown->getState() == STATE::ON ? VERRIERE_STATE::VERRIERE_MOVING_DOWN : VERRIERE_STATE::VERRIERE_STOP;

   // Si la verrière est à l'arrêt au démarrage, la cible est sa position actuelle.
  if (this->state == VERRIERE_STATE::VERRIERE_STOP) {
      this->target_position = this->current_position;
  } else {
      // Si elle est en mouvement au démarrage, la cible est inconnue pour l'instant.
      // On la met à la position actuelle par défaut, elle sera mise à jour par le thread de mouvement si nécessaire.
      this->target_position = this->current_position;
  }
  this->motion_start_time = std::chrono::steady_clock::now(); // Initialize
  this->last_mqtt_publish_time = std::chrono::steady_clock::now(); // Initialize

  this->init.store(false);
  this->stop_motion_thread.store(false); // Initialise le flag pour le thread de mouvement

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
            this->outputUp->setState(STATE::OFF);
            this->outputDown->setState(STATE::OFF); 
            this->stop_motion_thread.store(true); // Signale au thread de s'arrêter
            if (this->motion_thread.joinable()) {
                this->motion_thread.join(); // Attendre la fin du thread
            }
            // Lorsque la verrière s'arrête, nous mettons à jour la position actuelle
            if (oldState == VERRIERE_STATE::VERRIERE_MOVING_UP || oldState == VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
                 this->setCurrentPosition(this->calculateCurrentPositionBasedOnTime()); 
                 this->target_position = this->current_position; // La cible devient la position actuelle
            }
        break;
        case VERRIERE_STATE::VERRIERE_MOVING_DOWN:
            this->outputDown->setState(STATE::ON); 
            this->motion_start_time = std::chrono::steady_clock::now(); // Enregistrer le début du mouvement
            this->stop_motion_thread.store(false); // Réinitialiser le flag
            if (!this->motion_thread.joinable()) { // Démarrer le thread seulement s'il n'est pas déjà en cours
                this->motion_thread = std::thread(&VERRIERE::motionCalculationThread, this);
            }
        break;
        case VERRIERE_STATE::VERRIERE_MOVING_UP:
            this->outputUp->setState(STATE::ON); 
            this->motion_start_time = std::chrono::steady_clock::now(); // Enregistrer le début du mouvement
            this->stop_motion_thread.store(false); // Réinitialiser le flag
            if (!this->motion_thread.joinable()) { // Démarrer le thread seulement s'il n'est pas déjà en cours
                this->motion_thread = std::thread(&VERRIERE::motionCalculationThread, this);
            }
        break;
        case VERRIERE_STATE::VERRIERE_OPEN: // Cet état est atteint quand la position est 100
            this->outputUp->setState(STATE::OFF);
            this->outputDown->setState(STATE::OFF);
            this->setCurrentPosition(100);
            this->target_position = 100;
            this->stop_motion_thread.store(true); // Arrêter le thread de mouvement
            if (this->motion_thread.joinable()) {
                this->motion_thread.join();
            }
        break;
        case VERRIERE_STATE::VERRIERE_CLOSED: // Cet état est atteint quand la position est 0
            this->outputUp->setState(STATE::OFF);
            this->outputDown->setState(STATE::OFF);
            this->setCurrentPosition(0);
            this->target_position = 0;
            this->stop_motion_thread.store(true); // Arrêter le thread de mouvement
            if (this->motion_thread.joinable()) {
                this->motion_thread.join();
            }
            if (this->init.load()) { // Si c'était une phase d'initialisation, réinitialiser le flag
                this->init.store(false);
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
                    this->setState(VERRIERE_STATE::VERRIERE_STOP);
                }
            break;
            case VERRIERE_ACTION::VERRIERE_DOWN:
                if (this->getState() != VERRIERE_STATE::VERRIERE_MOVING_DOWN && this->getCurrentPosition() > 0) {
                    this->setTargetPosition(0); // Cible 0 pour une fermeture complète
                    this->setState(VERRIERE_STATE::VERRIERE_MOVING_DOWN);
                }
            break;
            case VERRIERE_ACTION::VERRIERE_UP:
                if (this->getState() != VERRIERE_STATE::VERRIERE_MOVING_UP && this->getCurrentPosition() < 100) {
                    this->setTargetPosition(100); // Cible 100 pour une ouverture complète
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
                            this->setState(VERRIERE_STATE::VERRIERE_MOVING_UP);
                        } else if (percent < this->getCurrentPosition()) {
                            this->setState(VERRIERE_STATE::VERRIERE_MOVING_DOWN);
                        }
                    } else { // Si la cible est la position actuelle, on s'arrête si on bougeait.
                        this->setState(VERRIERE_STATE::VERRIERE_STOP);
                    }
                }
            break;
            case VERRIERE_ACTION::VERRIERE_INIT:
                // L'initialisation doit être faite en déplaçant la verrière vers le bas jusqu'à ce qu'elle soit complètement fermée (position 0).
                // Cela réinitialise la position de référence.
                if (this->getState() != VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
                    this->init.store(true); // Active le mode initialisation
                    this->setTargetPosition(0); // La cible de l'initialisation est 0
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


// Fonction pour calculer la position basée sur le temps écoulé et les paramètres de mouvement
int VERRIERE::calculateCurrentPositionBasedOnTime() {
    auto now = std::chrono::steady_clock::now();
    long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->motion_start_time).count();

    int start_pos = this->current_position; // La position de départ était la dernière current_position au début du mouvement
    int new_position = this->current_position;

    VERRIERE_STATE currentState = this->getState();

    if (currentState == VERRIERE_STATE::VERRIERE_MOVING_UP) {
        long total_duration = this->getOpenDurationMs();
        long slowdown_duration = this->getOpenSlowdownDurationMs();

        if (total_duration == 0) return this->target_position; // Si durée est 0, on suppose que le mouvement est instantané à la cible.

        int distance_to_travel = this->target_position - start_pos;
        if (distance_to_travel <= 0) return start_pos; // Pas de mouvement UP si cible inférieure ou égale à l'actuelle

        float progress;
        if (elapsed_ms <= total_duration - slowdown_duration) {
            // Phase de vitesse normale
            progress = static_cast<float>(elapsed_ms) / (total_duration - slowdown_duration);
        } else {
            // Phase de ralentissement
            long slowdown_elapsed_ms = elapsed_ms - (total_duration - slowdown_duration);
            float slowdown_factor = static_cast<float>(slowdown_elapsed_ms) / slowdown_duration;
            // Utiliser une fonction de ralentissement, par exemple une interpolation cosinusoïdale pour un effet de ralentissement en douceur
            // progress = 1.0f - (1.0f - progress_before_slowdown) * (0.5f * (1.0f + std::cos(M_PI * slowdown_factor))); // Exemple
            // Pour l'instant, on utilise une interpolation linéaire simplifiée sur la phase de ralentissement.
            // On peut aussi considérer que la majeure partie de la distance est parcourue avant le ralentissement.
            
            // Calcul simplifié pour le ralentissement: la dernière partie de la distance est parcourue plus lentement.
            // Ce modèle peut être complexifié, ici une simple interpolation linéaire basée sur le temps total.
            progress = static_cast<float>(elapsed_ms) / total_duration;
        }
        
        progress = std::min(1.0f, std::max(0.0f, progress)); // Borner la progression entre 0 et 1

        new_position = start_pos + static_cast<int>(distance_to_travel * progress);

        // Assurer que la position ne dépasse pas la cible
        new_position = std::min(new_position, this->target_position);
        new_position = std::max(new_position, 0); // S'assurer que la position ne descend pas en dessous de 0
    } else if (currentState == VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
        long total_duration = this->getCloseDurationMs();
        long slowdown_duration = this->getCloseSlowdownDurationMs();

        if (total_duration == 0) return this->target_position; // Si durée est 0, on suppose que le mouvement est instantané à la cible.

        int distance_to_travel = start_pos - this->target_position; // Distance positive pour la descente (e.g., 100 -> 0, distance = 100)
        if (distance_to_travel <= 0) return start_pos; // Pas de mouvement DOWN si cible supérieure ou égale à l'actuelle

        float progress;
        if (elapsed_ms <= total_duration - slowdown_duration) {
            // Phase de vitesse normale
            progress = static_cast<float>(elapsed_ms) / (total_duration - slowdown_duration);
        } else {
            // Phase de ralentissement (même remarque que pour le mouvement UP)
            long slowdown_elapsed_ms = elapsed_ms - (total_duration - slowdown_duration);
            float slowdown_factor = static_cast<float>(slowdown_elapsed_ms) / slowdown_duration;
            progress = static_cast<float>(elapsed_ms) / total_duration;
        }

        progress = std::min(1.0f, std::max(0.0f, progress)); // Borner la progression entre 0 et 1

        new_position = start_pos - static_cast<int>(distance_to_travel * progress);

        // Assurer que la position ne dépasse pas la cible
        new_position = std::max(new_position, this->target_position);
        new_position = std::min(new_position, 100); // S'assurer que la position ne monte pas au-dessus de 100
    }
    
    // Assurer que la position reste dans les bornes [0, 100]
    new_position = std::max(0, std::min(100, new_position));
    return new_position;
}

// Fonction exécutée par le thread de mouvement
void VERRIERE::motionCalculationThread() {
    while (!stop_motion_thread.load()) {
        int oldPosition = this->getCurrentPosition();
        int newPosition = this->calculateCurrentPositionBasedOnTime();
        
        if (newPosition != oldPosition) {
            this->setCurrentPosition(newPosition);
        }

        // Vérifier si la cible est atteinte ou dépassée
        VERRIERE_STATE currentState = this->getState();
        if (currentState == VERRIERE_STATE::VERRIERE_MOVING_UP) {
            if (this->getCurrentPosition() >= this->getTargetPosition()) {
                if (this->getTargetPosition() == 100) {
                    this->setState(VERRIERE_STATE::VERRIERE_OPEN);
                } else {
                    this->setState(VERRIERE_STATE::VERRIERE_STOP);
                }
            }
        } else if (currentState == VERRIERE_STATE::VERRIERE_MOVING_DOWN) {
            // Logique pour les mouvements de descente (y compris l'initialisation)
            if (this->getCurrentPosition() <= this->getTargetPosition()) {
                // Si la cible (0) est atteinte, vérifier si c'est une initialisation
                if (this->getTargetPosition() == 0) {
                    // Calculer le temps écoulé depuis le début du mouvement
                    long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->motion_start_time).count();
                    
                    // Si c'est une initialisation ET que la durée de fermeture totale n'est pas encore écoulée,
                    // la verrière reste en mouvement (simulé) jusqu'à la fin de la durée.
                    // Sinon, elle passe à l'état CLOSED ou STOP.
                    if (this->init.load() && elapsed_ms < this->getCloseDurationMs()) {
                        // Rester dans l'état MOVING_DOWN et continuer la simulation jusqu'à la fin de la durée
                        // La position restera à 0 si elle l'a déjà atteinte, mais le mouvement "conceptuel" continue.
                        this->setCurrentPosition(0); // Assurer qu'elle est bien à 0
                    } else {
                        // Fin du mouvement ou de l'initialisation
                        this->setState(VERRIERE_STATE::VERRIERE_CLOSED);
                    }
                } else {
                    // Mouvement de descente vers une cible non nulle
                    this->setState(VERRIERE_STATE::VERRIERE_STOP);
                }
            }
        }

        // Publication MQTT périodique pour les mouvements
        auto now = std::chrono::steady_clock::now();
        long elapsed_since_last_publish = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->last_mqtt_publish_time).count();
        if (elapsed_since_last_publish >= 1000) { // Publier toutes les secondes pendant le mouvement
            this->setCurrentPosition(this->current_position);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Mettre à jour la position toutes les 50ms
    }
}


void VERRIERE::process() {
    // La logique principale de mise à jour de la position est maintenant dans motionCalculationThread.
    // Cette fonction peut être utilisée pour des mises à jour non liées au mouvement,
    // ou des publications d'état moins fréquentes si le thread de mouvement n'est pas actif.

    // Gestion de la publication MQTT périodique pour l'état actuel même sans changement majeur
    auto now = std::chrono::steady_clock::now();
    long elapsed_since_last_publish = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->last_mqtt_publish_time).count();
    // Publier l'état toutes les 5 secondes par exemple, ou si en mouvement toutes les secondes (géré par motionCalculationThread)
    long publish_interval_ms = (this->getState() == VERRIERE_STATE::VERRIERE_MOVING_UP || this->getState() == VERRIERE_STATE::VERRIERE_MOVING_DOWN) ? 1000 : 5000; 

    if (elapsed_since_last_publish >= publish_interval_ms && 
        !(this->getState() == VERRIERE_STATE::VERRIERE_MOVING_UP || this->getState() == VERRIERE_STATE::VERRIERE_MOVING_DOWN)) {
        json j;
        j["event"] = "STATUS_UPDATE"; // Un événement générique pour les mises à jour périodiques
        j["name"] = this->getName();
        j["comment"] = this->getComment();
        j["state"] = verriereState_to_string(this->getState());
        j["position"] = this->getCurrentPosition();
        j["target"] = this->getTargetPosition();
        this->getMqtt()->publish(this->getDispatchTOPIC(), j, false);
        this->last_mqtt_publish_time = now;
    }
}

void VERRIERE::_onMainThreadStart() {
    // Initialiser la position au démarrage de l'application si nécessaire
    // this->current_position = this->getConfigT()->current_position;
}


void VERRIERE::_onMainThreadStopping() {
    this->stop_motion_thread.store(true); // Assurer que le thread de mouvement s'arrête
    if (this->motion_thread.joinable()) {
        this->motion_thread.join();
    }
}

void VERRIERE::_onMainThreadStop() {

}


VERRIERE::~VERRIERE() {
    this->stop_motion_thread.store(true); // S'assurer que le thread est arrêté avant la destruction
    if (this->motion_thread.joinable()) {
        this->motion_thread.join();
    }
    this->outputUp->setState(STATE::OFF);
    this->outputDown->setState(STATE::OFF); // Assurez-vous que les deux sont éteintes à la destruction
}

/* =================================================================*/
class VERRIERES;

VERRIERES::VERRIERES(CONFIG* config, MyMqtt* myMqtt, Digital_Outputs *DOs) {
    for (auto verriereConf: config->getConfig()->verrieres->verrieres) {
      VERRIERE *V = new VERRIERE(config, verriereConf, myMqtt, DOs->findByName(verriereConf->up_DoName), DOs->findByName("verriereConf->down_DoName"));
      this->addVerriere(V);
      std::cout << "Added Verriere name: " << V->getName() << " comment: " << V->getComment() << std::endl;
    }
}

VERRIERES::~VERRIERES() {

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

}
void VERRIERES::stopChildrenThreads() {

}
void VERRIERES::joinChildrenThreads() {
    for(std::vector<VERRIERE*>::iterator it = std::begin(this->verrieres); it != std::end(this->verrieres); ++it) {
    (*it)->getProcessThread()->join();
  } 
}
