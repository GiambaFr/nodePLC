// verriere.cpp
#include "verriere.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm> // For std::min and std::max

Verriere::Verriere(const std::string& name,
                   const std::string& get_topic,
                   const std::string& set_topic,
                   const std::string& dispatch_topic,
                   int open_time_ms,
                   int close_time_ms,
                   int slowdown_time_ms)
    : name(name),
      get_TOPIC(get_topic),
      set_TOPIC(set_topic),
      dispatch_TOPIC(dispatch_topic),
      currentState(VERRIERE_STATE::VERRIERE_STOP),
      currentPosition(0), // Starts closed
      targetPosition(0),
      openTimeMs(open_time_ms),
      closeTimeMs(close_time_ms),
      slowdownTimeMs(slowdown_time_ms),
      isMoving(false) {
}

Verriere::~Verriere() {
    stop();
}

void Verriere::setMqttPublishCallback(std::function<void(const std::string& topic, const std::string& payload)> callback) {
    mqttPublishCallback = callback;
}

void Verriere::start() {
    if (!isMoving.load()) {
        isMoving.store(true);
        movementThread = std::thread(&Verriere::movementLoop, this);
    }
}

void Verriere::stop() {
    if (isMoving.load()) {
        isMoving.store(false);
        if (movementThread.joinable()) {
            movementThread.join();
        }
    }
}

void Verriere::handleMqttSet(const std::string& payload) {
    try {
        nlohmann::json j = nlohmann::json::parse(payload);
        std::string actionStr = j["action"];
        VERRIERE_ACTION action = verriereAction_from_string(actionStr);

        std::lock_guard<std::mutex> lock(verriereMutex); // Protect targetPosition

        if (action == VERRIERE_ACTION::VERRIERE_UP) {
            targetPosition.store(100);
            currentState.store(VERRIERE_STATE::VERRIERE_MOVING_UP);
            std::cout << "Verriere " << name << ": Set to open fully." << std::endl;
        } else if (action == VERRIERE_ACTION::VERRIERE_DOWN) {
            targetPosition.store(0);
            currentState.store(VERRIERE_STATE::VERRIERE_MOVING_DOWN);
            std::cout << "Verriere " << name << ": Set to close fully." << std::endl;
        } else if (action == VERRIERE_ACTION::VERRIERE_STOP) {
            targetPosition.store(currentPosition.load()); // Stop at current position
            currentState.store(VERRIERE_STATE::VERRIERE_STOP);
            std::cout << "Verriere " << name << ": Stopped." << std::endl;
        } else if (action == VERRIERE_ACTION::SET_PERCENTAGE) {
            int percentage = j["percentage"];
            if (percentage >= 0 && percentage <= 100) {
                targetPosition.store(percentage);
                if (percentage > currentPosition.load()) {
                    currentState.store(VERRIERE_STATE::VERRIERE_MOVING_UP);
                } else if (percentage < currentPosition.load()) {
                    currentState.store(VERRIERE_STATE::VERRIERE_MOVING_DOWN);
                } else {
                    currentState.store(VERRIERE_STATE::VERRIERE_STOP);
                }
                std::cout << "Verriere " << name << ": Set to " << percentage << "%." << std::endl;
            } else {
                std::cerr << "Invalid percentage value: " << percentage << std::endl;
            }
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parse error in Verriere::handleMqttSet: " << e.what() << std::endl;
    }
}

VERRIERE_STATE Verriere::getState() const {
    return currentState.load();
}

int Verriere::getPosition() const {
    return currentPosition.load();
}

void Verriere::publishPosition() {
    if (mqttPublishCallback) {
        nlohmann::json j;
        j["name"] = name;
        j["position"] = currentPosition.load();
        j["state"] = verriereState_to_string(currentState.load());
        mqttPublishCallback(dispatch_TOPIC, j.dump());
    }
}

void Verriere::movementLoop() {
    auto lastUpdateTime = std::chrono::steady_clock::now();
    int lastPublishedPosition = currentPosition.load();

    while (isMoving.load()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();

        if (currentState.load() != VERRIERE_STATE::VERRIERE_STOP) {
            int currentPos = currentPosition.load();
            int targetPos = targetPosition.load();
            int deltaPos = targetPos - currentPos;

            if (deltaPos != 0) {
                double timePerPercent;
                if (deltaPos > 0) { // Opening
                    timePerPercent = static_cast<double>(openTimeMs) / 100.0;
                } else { // Closing
                    timePerPercent = static_cast<double>(closeTimeMs) / 100.0;
                }

                double theoreticalDelta = elapsed / timePerPercent;
                int newPosition = currentPos;

                if (deltaPos > 0) { // Opening movement
                    newPosition = static_cast<int>(currentPos + theoreticalDelta);
                    newPosition = std::min(newPosition, targetPos);
                } else { // Closing movement
                    // Apply slowdown at the end of closing
                    // Slowdown starts when position is below (slowdownTimeMs / closeTimeMs) * 100%
                    double slowdownThresholdPercentage = (static_cast<double>(slowdownTimeMs) / closeTimeMs) * 100.0;

                    if (currentPos <= slowdownThresholdPercentage && currentPos > 0) {
                        // Slowdown factor: closer to 0 means slower movement
                        double slowdownFactor = (static_cast<double>(currentPos) / slowdownThresholdPercentage);
                        if (slowdownFactor < 0.1) slowdownFactor = 0.1; // Ensure a minimum movement speed
                        newPosition = static_cast<int>(currentPos - theoreticalDelta * slowdownFactor);
                    } else {
                        newPosition = static_cast<int>(currentPos - theoreticalDelta);
                    }
                    newPosition = std::max(newPosition, targetPos);
                }

                currentPosition.store(newPosition);

                // Check if target is reached
                if ((deltaPos > 0 && currentPosition.load() >= targetPos) ||
                    (deltaPos < 0 && currentPosition.load() <= targetPos)) {
                    currentPosition.store(targetPos); // Ensure it lands exactly on target
                    if (currentPosition.load() == 0) {
                        currentState.store(VERRIERE_STATE::VERRIERE_CLOSED);
                    } else if (currentPosition.load() == 100) {
                        currentState.store(VERRIERE_STATE::VERRIERE_OPEN);
                    } else {
                        currentState.store(VERRIERE_STATE::VERRIERE_STOP);
                    }
                    std::cout << "Verriere " << name << ": Reached target position " << currentPosition.load() << "%." << std::endl;
                }
            } else {
                if (currentPosition.load() == 0) {
                    currentState.store(VERRIERE_STATE::VERRIERE_CLOSED);
                } else if (currentPosition.load() == 100) {
                    currentState.store(VERRIERE_STATE::VERRIERE_OPEN);
                } else {
                    currentState.store(VERRIERE_STATE::VERRIERE_STOP);
                }
            }
        }

        // Publish position every second or if stopped and position changed since last publish
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count() >= 1000 ||
            (currentState.load() == VERRIERE_STATE::VERRIERE_STOP && lastPublishedPosition != currentPosition.load())) {
            publishPosition();
            lastPublishedPosition = currentPosition.load();
            lastUpdateTime = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Update every 100ms for smoother movement
    }
}