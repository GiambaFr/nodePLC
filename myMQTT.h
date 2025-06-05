#pragma once
#ifndef MYMQTT_H
#define MYMQTT_H

#include <iostream>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>
#include "mqtt/async_client.h"

#include "config.h"

using json = nlohmann::json;
using MQTT = mqtt::async_client;


class MyMqtt {
    private:
         CONFIG *config;
         mqtt::async_client *cli;
         bool connected = false;
         std::string willTopic;
         std::string willMessage;
         std::string onlineMessage;
         using messageArriveFunc = std::function<void(std::string, std::string)>;
         std::vector<messageArriveFunc> messageArriveHandlers;
         /*using nodeRedConnectedFunc = std::function<void()>;
         std::vector<nodeRedConnectedFunc> nodeRedConnectedHandlers;*/

    public:
        MyMqtt(CONFIG *config);
        ~MyMqtt();
        CONFIG *getConfig();
        void subscribe();
        void _onMessageArrived(std::string topic, std::string payload);
        void _onNodeRedConnected();
        bool connect();
        void disconnect();
        void publish(std::string topic, json payload, bool retain);
        void publish(std::string topic, std::string payload, bool retain);
        void publish(std::string topic, char* payload, bool retain);


        void addMessageArrivedHandler(messageArriveFunc);
        /*void addNodeRedConnectedHandler(nodeRedConnectedFunc);*/

        std::vector<std::string> splitTopic (const std::string &s);
};

//extern MyMqtt *myMqtt;

#endif