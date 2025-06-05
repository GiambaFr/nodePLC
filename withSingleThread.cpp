
#include <chrono>

#include "withSingleThread.h"


class withSingleThread;


withSingleThread::withSingleThread() {}


void withSingleThread::process() {
 // user define function
}

void withSingleThread::setThreadSleepTimeMicro(int st) {
    this->sleepTimeMicro = st;
}

void withSingleThread::setThreadSleepTimeMillis(int st) {
    this->sleepTimeMicro = st * 1000;
}

std::thread withSingleThread::processThread() { 
    return std::thread([&] {
        while (this->run) {
            this->process();
            std::this_thread::sleep_for(std::chrono::microseconds(this->sleepTimeMicro));
        }
        this->_onMainThreadStop();
    });
}


void withSingleThread::start(const char *name) {
    if (!this->run) {
        this->_onMainThreadStart();
        this->run = true;
        this->main_thread = this->processThread();
        pthread_setname_np(this->main_thread.native_handle(), name);
    }
}


void withSingleThread::stop() {
    if (this->run) {
        this->_onMainThreadStopping();
        this->run = false;
    }
}


void withSingleThread::_onMainThreadStart() {
    //user define function
}

void withSingleThread::_onMainThreadStopping() {
    //user define function
}

void withSingleThread::_onMainThreadStop() {
    //user define function
}

std::thread *withSingleThread::getProcessThread() {
    return &this->main_thread;
}


withSingleThread::~withSingleThread() {
    if (this->run)
        this->run = false;
}

