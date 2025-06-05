#pragma once
#ifndef WITH_CONFIG_H
#define WITH_CONFIG_H

#include "config.h"
#include "withMqtt.h"

template<typename ConfigT>
class withConfig {
    private:   
        CONFIG *config;
        ConfigT *configT;
    protected:
        void setConfig(CONFIG*);
        CONFIG *getConfig();
        void setConfigT(ConfigT*);
        ConfigT *getConfigT();
        void saveConfig();

        std::string getGetTOPIC();
        void setGetTOPIC(std::string);
        std::string getSetTOPIC();
        void setSetTOPIC(std::string);
        std::string getDispatchTOPIC();
        void setDispatchTOPIC(std::string);

        template<typename ParamT>
        json getSendBackParams(std::string /*param*/, ParamT value);

    public:
        withConfig();
        withConfig(CONFIG*, ConfigT*);
        virtual ~withConfig();
        
        std::string getName();
        void setName(std::string);
        std::string getComment();
        void setComment(std::string);

        void setBaseParams(json);
};

#endif