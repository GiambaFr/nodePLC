#ifndef COMMONS_H
#define COMMONS_H
#include <iostream>
#include <sstream>
#include <vector>
#include <string> // Ensure string is included


using std::vector;
using std::string;
using std::stringstream;


enum class STATE {NOT_SET, OFF, ON, DIM};
std::string state_to_string(STATE state);
STATE state_from_string(std::string);

enum class VMC_STATE {NOT_SET, PV, GV};
std::string vmcState_to_string(VMC_STATE state);
VMC_STATE vmcState_from_string(std::string);


enum class VERRIERE_STATE { NOT_SET, VERRIERE_STOP, VERRIERE_MOVING_DOWN, VERRIERE_MOVING_UP, VERRIERE_OPEN, VERRIERE_CLOSED };
std::string verriereState_to_string(VERRIERE_STATE state);
VERRIERE_STATE verriereState_from_string(std::string s);

enum class VERRIERE_ACTION { VERRIERE_STOP, VERRIERE_DOWN, VERRIERE_UP, SET_PERCENTAGE, VERRIERE_INIT };
std::string verriereAction_to_string(VERRIERE_ACTION verriereAction);
VERRIERE_ACTION verriereAction_from_string(std::string s);

//enum CE_STATE {STOP, HEAT};

inline const char * const BoolToString(bool b)
{
  return b ? "true" : "false";
}

enum class OUTPUT_TYPE { RELAY, DIMMABLE, TIMED, VERRIERE };
std::string outputType_to_string(OUTPUT_TYPE outputYype);
OUTPUT_TYPE outputType_from_string(std::string s);

enum class LIGHT_TYPE { DIMMABLE, ONOFF };
std::string lightType_to_string(LIGHT_TYPE lightType);
LIGHT_TYPE lightType_from_string(std::string s);

enum class LIGHT_ACTION { TOGGLE, MAX, DIM_START, DIM_STOP, ON, OFF, SET, MEMO };
std::string lightAction_to_string(LIGHT_ACTION lightAction);
LIGHT_ACTION lightAction_from_string(std::string s);


enum class BUTTON_TYPE {PUSH_BUTTON, ONOFF_BUTTON};
enum class BUTTON_ACTION {PRESSED, RELEASED, SIMPLECLICK, MULTICLICK, LONGCLICKSTART, LONGCLICKSTOP};
enum class CLICK_TYPE { NOCLICK, SIMPLECLICK, MULTICLICK, LONGCLICK };

std::string btAction_to_string(BUTTON_ACTION btAction);
BUTTON_ACTION btAction_from_string(std::string s);

std::string btType_to_string(BUTTON_TYPE btType);
BUTTON_TYPE btType_from_string(std::string typeStr);

enum class CHAUFFE_EAU_MODE {NOT_SET, SUN, ELECTRIC, FIOUL};
std::string chauffe_eau_mode_to_string(CHAUFFE_EAU_MODE mode);
CHAUFFE_EAU_MODE chauffe_eau_mode_from_string(std::string s);


enum class CHAUFFAGE_MODE {NOT_SET, ETE, HIVER};
std::string chauffage_mode_to_string(CHAUFFAGE_MODE mode);
CHAUFFAGE_MODE chauffage_mode_from_string(std::string s);

enum class CHAUFFAGE_SOURCE {NOT_SET, FIOUL, CHAUFFE_EAU};
std::string chauffage_source_to_string(CHAUFFAGE_SOURCE mode);
CHAUFFAGE_SOURCE chauffage_source_from_string(std::string s);

enum class V3V_STATE {NOT_SET, STOP, UP, DOWN};
std::string v3v_state_to_string(V3V_STATE mode);
V3V_STATE v3v_state_from_string(std::string s);

#endif