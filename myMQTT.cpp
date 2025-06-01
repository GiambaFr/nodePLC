#include <iostream>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <sstream>
#include <vector>
#include "mqtt/async_client.h"


#include "threadPool.h"
#include "config.h"
#include "myMQTT.h"

using json = nlohmann::json;
using MQTT = mqtt::async_client;


const auto TIMEOUT = std::chrono::seconds(10);


MyMqtt::MyMqtt(CONFIG *config) : config(config) {
    this->willTopic = config->getConfig()->mqtt->willTopic;
    this->willMessage = config->getConfig()->mqtt->willMessage;
    this->onlineMessage = config->getConfig()->mqtt->onlineMessage;
    this->cli = new mqtt::async_client(config->getConfig()->mqtt->host, config->getConfig()->mqtt->clientId);
    this->cli->set_message_callback([this](mqtt::const_message_ptr msg){
        this->_onMessageArrived(msg->get_topic(), msg->to_string());
    });
    this->cli->set_connection_lost_handler([this](const std::string& cause){
        std::cout << "MQTT Disconnected..." << std::endl;
        this->connected = false;
        this->connect();
    });
    
    /*this->addMessageArrivedHandler([this](std::string topic, std::string payload){
        if (topic == "HousePLC/NodeRed/status") {
            json j = json::parse(payload);
            if(j.contains("status") && j["status"].get<std::string>() == "online") {
                this->_onNodeRedConnected();
            }
        }
    });*/

    //this->connect();
}

CONFIG *MyMqtt::getConfig() {
    return this->config;
}

bool MyMqtt::connect() {
    auto connOpts = mqtt::connect_options_builder()
		.clean_session()
		.will(mqtt::message(this->willTopic, this->willMessage.c_str(), 1, true))
		.finalize();
    try {
		std::cout << "MQTT Connecting..." << std::endl;
		mqtt::token_ptr conntok = this->cli->connect(connOpts);
		std::cout << "MQTT Waiting for the connection..." << std::endl;
		conntok->wait();
		std::cout << "MQTT Connected" << std::endl;
        this->connected = true;
        this->subscribe();
        this->publish(this->willTopic, this->onlineMessage, true);
        return true;
    } catch (const mqtt::exception& exc) {
		std::cerr << exc.what() << std::endl;
		return false;
	}
}

void MyMqtt::subscribe() {
    for(auto st: this->config->getConfig()->mqtt->suscribeTopics) {
        this->cli->subscribe(st, 1);
    }
}

void MyMqtt::_onMessageArrived(std::string topic, std::string payload) {
  for(auto&& handler : this->messageArriveHandlers) {
    threadPool.push_task(handler, topic, payload);
    //std::thread t(handler, topic, payload);
    //t.detach();
  }    
}

/*void MyMqtt::_onNodeRedConnected() {
  for(auto&& handler : this->nodeRedConnectedHandlers) {
    std::thread t(handler);
    t.detach();
  }     
}*/

void MyMqtt::disconnect() {
    if (this->connected) {
        std::cout << "MQTT Disconnecting..." << std::endl;
        this->cli->disconnect()->wait();
        std::cout << "MQTT Disconnected" << std::endl;
    } else {
        std::cout << "Try to disconnected MQTT but not connected !" << std::endl;
    }
}

void MyMqtt::publish(std::string topic, json payload, bool retain) {
    if (this->connected) {
        mqtt::delivery_token_ptr pubtok;
        pubtok = this->cli->publish(topic, payload.dump().c_str(), strlen(payload.dump().c_str()), 1, retain);
        //std::cout << "  ...with token: " << pubtok->get_message_id() << std::endl;
        //std::cout << "  ...for message with " << pubtok->get_message()->get_payload().size() << " bytes" << std::endl;
        pubtok->wait_for(TIMEOUT);
        //std::cout << "  ...OK" << std::endl;
    }
}

void MyMqtt::publish(std::string topic, std::string payload, bool retain) {
    if (this->connected) {
        mqtt::delivery_token_ptr pubtok;
        pubtok = this->cli->publish(topic, payload.c_str(), strlen(payload.c_str()), 1, retain);
        //std::cout << "  ...with token: " << pubtok->get_message_id() << std::endl;
        //std::cout << "  ...for message with " << pubtok->get_message()->get_payload().size() << " bytes" << std::endl;
        pubtok->wait_for(TIMEOUT);
        //std::cout << "  ...OK" << std::endl;
    }
}

void MyMqtt::publish(std::string topic, char* payload, bool retain) {
    if (this->connected) {
        mqtt::delivery_token_ptr pubtok;
        pubtok = this->cli->publish(topic, payload, strlen(payload), 1, retain);
        //std::cout << "  ...with token: " << pubtok->get_message_id() << std::endl;
        //std::cout << "  ...for message with " << pubtok->get_message()->get_payload().size() << " bytes" << std::endl;
        pubtok->wait_for(TIMEOUT);
        //std::cout << "  ...OK" << std::endl;
    }
}


void MyMqtt::addMessageArrivedHandler(messageArriveFunc function) {
    this->messageArriveHandlers.push_back(function);
}

/*void MyMqtt::addNodeRedConnectedHandler(nodeRedConnectedFunc function) {
    this->nodeRedConnectedHandlers.push_back(function);
}*/

std::vector<std::string> MyMqtt::splitTopic (const std::string &s) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;
    char delim = '/';

    while ( getline (ss, item, delim) ) {
        result.push_back (item);
    }

    return result;
}

MyMqtt::~MyMqtt() {
    this->disconnect();
    delete this->cli;
}