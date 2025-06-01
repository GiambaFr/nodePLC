#ifndef CHAUFDIRE_H
#define CHAUFDIRE_H

#include <stdlib.h>

#include "digital_input.h"
#include "digital_output.h"
#include "temp_sensors.h"

#define consoHG (2.85)


class Chaudiere {
    private:
        Digital_Output *Bruleur_Output;
        Digital_Input *Ev_Input;
        Digital_Input *Default_Input;
        float tempSetPoint;
        Temp_Sensor temp;
        float fioulQte;

    public:
        Chaudiere();
        ~Chaudiere();
};

#endif