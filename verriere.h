#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

#include "commons.h" // Assurez-vous que commons.h contient les définitions d'énumération

class Verriere {
public:
    // Constructor
    Verriere(const std::string& name,
             const std::string& get_topic,
             const std::string& set_topic,
             const std::string& dispatch_topic,
             int open_time_ms,
             int close_time_ms,
             int slowdown_time_ms);
    // Destructor
    ~Verriere();

    // Processes incoming MQTT command messages
    void handleMqttSet(const std::string& payload);
    // Returns the current state of the verriere
    VERRIERE_STATE getState() const;
    // Returns the current position in percentage (0-100)
    int getPosition() const;

    // Sets the callback function for MQTT publishing
    void setMqttPublishCallback(std::function<void(const std::string& topic, const std::string& payload)> callback);

    // Starts the verriere's internal thread
    void start();
    // Stops the verriere's internal thread
    void stop();

private:
    std::string name;
    std::string get_TOPIC;
    std::string set_TOPIC;
    std::string dispatch_TOPIC;

    std::atomic<VERRIERE_STATE> currentState;
    std::atomic<int> currentPosition; // 0 = closed, 100 = open
    std::atomic<int> targetPosition;

    int openTimeMs; // Total time to open from 0 to 100%
    int closeTimeMs; // Total time to close from 100 to 0%
    int slowdownTimeMs; // Time for slowdown at the end of closing

    std::atomic<bool> isMoving;
    std::thread movementThread;
    std::mutex verriereMutex; // Mutex to protect shared variables
    std::function<void(const std::string& topic, const std::string& payload)> mqttPublishCallback;

    // Main function for the movement thread
    void movementLoop();
    // Publishes the current position via MQTT
    void publishPosition();
};