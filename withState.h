#ifndef WITH_STATE_H
#define WITH_STATE_H

#include <vector>
#include <iostream>
#include <functional>

#include "commons.h"


template<typename StateType>
class withState {
    private:
        int dimValue = 0;
        int dimCycleTime = 200;
    protected:
        StateType state = StateType(0); //NOT_SET
        using stateChangeHandlersFunc = std::function<void(StateType oldState, StateType newState)>;
        std::vector<stateChangeHandlersFunc> stateChangeHandlers;
        void _onStateChange(StateType oldState, StateType newState);
        void setState(StateType /*state*/);
        StateType getState();
        void addStateChangeHandler(stateChangeHandlersFunc /*function*/);

        using dimValueChangeHandlersFunc = std::function<void(int oldDimValue, int newDimValue)>;
        std::vector<dimValueChangeHandlersFunc> dimValueChangeHandlers;
        void _onDimValueChange(int oldDimValue, int newDimValue);
        void setDimValue(int);
        int getDimValue();
        void addDimValueChangeHandler(dimValueChangeHandlersFunc /*function*/);

        using dimCycleTimeChangeHandlersFunc = std::function<void(int oldDimCycleTime, int newDimCycleTime)>;
        std::vector<dimCycleTimeChangeHandlersFunc> dimCycleTimeChangeHandlers;
        void _onDimCycleTimeChange(int oldDimCycleTime, int newDimCycleTime);            
        void setDimCycleTime(int);
        int getDimCycleTime();
        void addDimCycleTimeChangeHandler(dimCycleTimeChangeHandlersFunc /*function*/);

    public:
        withState();
        virtual ~withState();
};


#endif