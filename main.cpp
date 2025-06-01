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
//#include "buttons.h"
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
    // cleanup and close up stuff here
    lights->stopChildrenThreads();
    //buttons->stopChildrenThreads();
    TempSensors->stopChildrenThreads();
    Outputs->stopChildrenThreads();
    Inputs->stopChildrenThreads();
    //Wait for COUT
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    delete lights;
    //delete buttons;
    delete TempSensors;
    delete Outputs;
    delete Inputs;
    delete myMqtt;
    delete config;
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
    signal(SIGPIPE, SIG_IGN);
    std::atexit(atexit_handler);

    config = new CONFIG();
    cout << "test" << endl;
    //cout.flush();
    //cout << argv[1] << "/config.json" << endl;
    //cout << "test" << endl;
    //cout.flush();
    //std::string path = argv[1];
    //config->load(path + "/config.json");
    config->load("/root/projects/nodePLC/build/config.json");
    //cout << "test" << endl;
    //cout.flush();

    myMqtt = new MyMqtt(config);
    Inputs = new Digital_Inputs(config, myMqtt);
    Outputs = new Digital_Outputs(config, myMqtt);
    TempSensors = new Temp_Sensors(config, myMqtt);
    //buttons = new Buttons(config, myMqtt);
    lights = new LIGHTS(config, myMqtt);

    Inputs->startChildrenThreads();
    Outputs->startChildrenThreads(); // for dimmable output, need of thread for pwm
    TempSensors->startChildrenThreads();
    //buttons->startChildrenThreads();
    lights->startChildrenThreads();

    myMqtt->connect();


    //Wait for thread to stop
    lights->joinChildrenThreads();
    //buttons->joinChildrenThreads();
    TempSensors->joinChildrenThreads();
    Outputs->joinChildrenThreads();
    Inputs->joinChildrenThreads();

    return EXIT_SUCCESS;
}