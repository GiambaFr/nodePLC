#include <iostream>
#include <vector>
#include <algorithm>

#include "commons.h"


std::vector<std::string> stateStr = {"NOT_SET", "OFF", "ON", "DIM"};
std::string state_to_string(STATE state) {return stateStr[int(state)];}
STATE state_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(stateStr.begin(), stateStr.end(), s);
    return STATE(std::distance(stateStr.begin(), itr));
}

std::vector<std::string> vmcStateStr = {"NOT_SET", "PV", "GV"};
std::string vmcState_to_string(VMC_STATE state) {return vmcStateStr[int(state)];}
VMC_STATE vmcState_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(vmcStateStr.begin(), vmcStateStr.end(), s);
    return VMC_STATE(std::distance(vmcStateStr.begin(), itr));
}

std::vector<std::string> verriereStateStr = {"NOT_SET", "VERRIERE_STOP", "VERRIERE_MOVING_DOWN", "VERRIERE_MOVING_UP", "VERRIERE_OPEN", "VERRIERE_CLOSED"};
std::string verriereState_to_string(VERRIERE_STATE state) { return verriereStateStr[static_cast<int>(state)]; }
VERRIERE_STATE verriereState_from_string(std::string s) {
    auto itr = std::find(verriereStateStr.begin(), verriereStateStr.end(), s);
    if (itr == verriereStateStr.end()) {
        // Handle error, e.g., throw an exception or return a default
        return VERRIERE_STATE::NOT_SET;
    }
    return static_cast<VERRIERE_STATE>(std::distance(verriereStateStr.begin(), itr));
}

std::vector<std::string> verriereActionStr = {"VERRIERE_STOP", "VERRIERE_DOWN", "VERRIERE_UP", "SET_PERCENTAGE"};
std::string verriereAction_to_string(VERRIERE_ACTION verriereAction) { return verriereActionStr[static_cast<int>(verriereAction)]; }
VERRIERE_ACTION verriereAction_from_string(std::string s) {
    auto itr = std::find(verriereActionStr.begin(), verriereActionStr.end(), s);
    if (itr == verriereActionStr.end()) {
        // Handle error
        return VERRIERE_ACTION::VERRIERE_STOP; // Or throw
    }
    return static_cast<VERRIERE_ACTION>(std::distance(verriereActionStr.begin(), itr));
}


std::vector<std::string> outputTypeStr = {"RELAY", "DIMMABLE", "TIMED", "VERRIERE"};
std::string outputType_to_string(OUTPUT_TYPE outputType) {return outputTypeStr[int(outputType)];};
OUTPUT_TYPE outputType_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(outputTypeStr.begin(), outputTypeStr.end(), s);
    return OUTPUT_TYPE(std::distance(outputTypeStr.begin(), itr));
};

std::vector<std::string> lightTypeStr = {"DIMMABLE", "ONOFF"};
std::string lightType_to_string(LIGHT_TYPE lightType) {return lightTypeStr[int(lightType)];};
LIGHT_TYPE lightType_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(lightTypeStr.begin(), lightTypeStr.end(), s);
    return LIGHT_TYPE(std::distance(lightTypeStr.begin(), itr));
};

std::vector<std::string> lightActionStr = {"TOGGLE", "MAX", "DIM_START", "DIM_STOP", "ON", "OFF", "SET", "MEMO"};
std::string lightAction_to_string(LIGHT_ACTION lightAction) {return lightActionStr[int(lightAction)];};
LIGHT_ACTION lightAction_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(lightActionStr.begin(), lightActionStr.end(), s);
    return LIGHT_ACTION(std::distance(lightActionStr.begin(), itr));
};


std::vector<std::string> buttonActionStr = {"PRESSED", "RELEASED", "SIMPLECLICK", "MULTICLICK", "LONGCLICKSTART", "LONGCLICKSTOP"};
std::string btAction_to_string(BUTTON_ACTION btAction) {return buttonActionStr[int(btAction)];};
BUTTON_ACTION btAction_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(buttonActionStr.begin(), buttonActionStr.end(), s);
    return BUTTON_ACTION(std::distance(buttonActionStr.begin(), itr));
};

std::vector<std::string> buttonTypeStr = { "PUSH_BUTTON", "ONOFF_BUTTON" };
std::string btType_to_string(BUTTON_TYPE btType) {return buttonTypeStr[int(btType)];};
BUTTON_TYPE btType_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(buttonTypeStr.begin(), buttonTypeStr.end(), s);
    return BUTTON_TYPE(std::distance(buttonTypeStr.begin(), itr));
};


std::vector<std::string> chauffe_eauModeStr = {"NOT_SET", "SUN", "ELECTRIC", "FIOUL"};
std::string chauffe_eau_mode_to_string(CHAUFFE_EAU_MODE mode) {return chauffe_eauModeStr[int(mode)];}
CHAUFFE_EAU_MODE chauffe_eau_mode_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(chauffe_eauModeStr.begin(), chauffe_eauModeStr.end(), s);
    return CHAUFFE_EAU_MODE(std::distance(chauffe_eauModeStr.begin(), itr));
}



std::vector<std::string> chauffageModeStr = {"NOT_SET", "ETE", "HIVER"};
std::string chauffage_mode_to_string(CHAUFFAGE_MODE mode) {return chauffageModeStr[int(mode)];}
CHAUFFAGE_MODE chauffage_mode_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(chauffageModeStr.begin(), chauffageModeStr.end(), s);
    return CHAUFFAGE_MODE(std::distance(chauffageModeStr.begin(), itr));
}


std::vector<std::string> chauffageSourceStr = {"NOT_SET", "FIOUL", "CHAUFFE_EAU"};
std::string chauffage_source_to_string(CHAUFFAGE_SOURCE source) {return chauffageSourceStr[int(source)];}
CHAUFFAGE_SOURCE chauffage_source_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(chauffageSourceStr.begin(), chauffageSourceStr.end(), s);
    return CHAUFFAGE_SOURCE(std::distance(chauffageSourceStr.begin(), itr));
}


std::vector<std::string> v3vStateStr = {"NOT_SET", "STOP", "UP", "DOWN"};
std::string v3v_state_to_string(V3V_STATE state) {return v3vStateStr[int(state)];}
V3V_STATE v3v_state_from_string(std::string s) {
    std::vector<std::string>::iterator itr = std::find(v3vStateStr.begin(), v3vStateStr.end(), s);
    return V3V_STATE(std::distance(v3vStateStr.begin(), itr));
}