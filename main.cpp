#include <iostream>
#include <fstream>
#include <streambuf>
#include <cstdlib>


#define JSON_TRY_USER if(true)
#define JSON_CATCH_USER(exception) if(false)
#define JSON_THROW_USER(exception)                           \
    {std::clog << "Error in " << __FILE__ << ":" << __LINE__ \
               << " (function " << __FUNCTION__ << ") - "    \
               << (exception).what() << std::endl;           \
     std::abort();}

#include <nlohmann/json.hpp>
#include <chrono>
#include <csignal>
#include <unistd.h>

#include "threadPool.h"

#include "config.h"
#include "commons.h"
#include "myMQTT.h"
#include "digital_input.h"
#include "digital_output.h"
#include "temp_sensors.h"
#include "light.h"

// for convenience
using json = nlohmann::json;

using namespace std;

BS::thread_pool_light threadPool(1);

CONFIG *config;

Digital_Inputs * Inputs;
Digital_Outputs * Outputs;
Temp_Sensors * TempSensors;
MyMqtt *myMqtt;
//Buttons *buttons;
LIGHTS *lights;


void signalHandler( int signum ) {
   std::cout << "Interrupt signal (" << signum << ") received.\n";
   // terminate program  
   exit(EXIT_SUCCESS);  
}

void atexit_handler() 
{
    std::cout << "Exiting application. Cleaning up resources..." << std::endl;

    // cleanup and close up stuff here
    if (lights) {
        lights->stopChildrenThreads(); // Assurez-vous que cette méthode existe
        delete lights;
        lights = nullptr;
    }
     if (TempSensors) {
        TempSensors->stopChildrenThreads(); // Assurez-vous que cette méthode existe
        delete TempSensors;
        TempSensors = nullptr;
    }
    if (Outputs) {
        Outputs->stopChildrenThreads(); // Assurez-vous que cette méthode existe
        delete Outputs;
        Outputs = nullptr;
    }
    if (Inputs) {
        Inputs->stopChildrenThreads(); // Assurez-vous que cette méthode existe
        delete Inputs;
        Inputs = nullptr;
    }
    if (myMqtt) {
        // Supposons que votre MyMqtt a une méthode pour désabonner ou déconnecter
        delete myMqtt;
        myMqtt = nullptr;
    }
    if (config) {
        // Le destructeur de CONFIG devrait gérer la désallocation de ses membres (mqtt, inputs, etc.)
        delete config;
        config = nullptr;
    }
    std::cout << "Cleanup complete." << std::endl;
}
 
std::string getCWD(char *argv0) {
    std::string arg = std::string(argv0);
    return arg.substr(0, arg.length() - 9); 
}

int main(int argc, char** argv) {

    // register signal SIGINT and signal handler  
    signal( SIGINT, signalHandler );  
    signal( SIGTERM, signalHandler );
    signal( SIGKILL, signalHandler );
    //signal(SIGPIPE, SIG_IGN);
    std::atexit(atexit_handler);

    config = new CONFIG();
    cout << "Loading configuration..." << endl;
    config->load("/root/projects/nodePLC/build/config.json");
    cout << "Configuration loaded." << endl;

    myMqtt = new MyMqtt(config);
    Inputs = new Digital_Inputs(config, myMqtt);
    Outputs = new Digital_Outputs(config, myMqtt);
    TempSensors = new Temp_Sensors(config, myMqtt);
    lights = new LIGHTS(config, myMqtt);


    Inputs->startChildrenThreads();
    Outputs->startChildrenThreads(); // for dimmable output, need of thread for pwm
    TempSensors->startChildrenThreads();
    lights->startChildrenThreads();

    myMqtt->connect();


    //Wait for thread to stop
    //lights->joinChildrenThreads();
    //TempSensors->joinChildrenThreads();
    //Outputs->joinChildrenThreads();
    //Inputs->joinChildrenThreads();

    return EXIT_SUCCESS;
}