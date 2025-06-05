#include <thread>

#include "commons.h"
#include "withState.h"
#include "threadPool.h"

template<typename StateType>
withState<StateType>::withState() {
    
}

//State

template<typename StateType>
void withState<StateType>::_onStateChange(StateType oldState, StateType newState) {
    for(auto&& handler : this->stateChangeHandlers) {
        //std::thread(handler, oldState, newState).detach();
        threadPool.push_task(handler, oldState, newState);
    }
}

template<typename StateType>
void withState<StateType>::setState(StateType state) {
    if (this->state != state) {
        this->_onStateChange(this->state, state);
        this->state = state;
    }
}

template<typename StateType>
StateType withState<StateType>::getState() {
    return this->state;
}

template<typename StateType>
void withState<StateType>::addStateChangeHandler(stateChangeHandlersFunc function) {
    this->stateChangeHandlers.push_back(function);
}

//dimValue

template<typename StateType>
void withState<StateType>::_onDimValueChange(int oldDimValue, int newDimValue) {
    for(auto&& handler : this->dimValueChangeHandlers) {
        //std::thread(handler, oldState, newState).detach();
        threadPool.push_task(handler, oldDimValue, newDimValue);
    }
}

template<typename StateType>
void withState<StateType>::setDimValue(int dimValue) {
    if (this->dimValue != dimValue) {
        this->_onDimValueChange(this->dimValue, dimValue);
        this->dimValue = dimValue;
    }
}

template<typename StateType>
int withState<StateType>::getDimValue() {
    return this->dimValue;
}

template<typename StateType>
void withState<StateType>::addDimValueChangeHandler(dimValueChangeHandlersFunc function) {
    this->dimValueChangeHandlers.push_back(function);
}


//dimCycleTime DimCycleTime

template<typename StateType>
void withState<StateType>::_onDimCycleTimeChange(int oldDimCycleTime, int newDimCycleTime) {
    for(auto&& handler : this->dimCycleTimeChangeHandlers) {
        //std::thread(handler, oldState, newState).detach();
        threadPool.push_task(handler, oldDimCycleTime, newDimCycleTime);
    }
}

template<typename StateType>
void withState<StateType>::setDimCycleTime(int dimCycleTime) {
    if (this->dimCycleTime != dimCycleTime) {
        this->_onDimCycleTimeChange(this->dimCycleTime, dimCycleTime);
        this->dimCycleTime = dimCycleTime;
    }
}

template<typename StateType>
int withState<StateType>::getDimCycleTime() {
    return this->dimCycleTime;
}

template<typename StateType>
void withState<StateType>::addDimCycleTimeChangeHandler(dimCycleTimeChangeHandlersFunc function) {
    this->dimCycleTimeChangeHandlers.push_back(function);
}



template<typename StateType>
withState<StateType>::~withState() {}

template class withState<STATE>;
template class withState<VERRIERE_STATE>;