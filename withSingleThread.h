#ifndef SINGLETHREADCONTAINER_H
#define SINGLETHREADCONTAINER_H

#include <thread>

class withSingleThread {
  private:  
        std::thread main_thread;
        std::thread processThread();
  protected:
        int sleepTimeMicro = 500*1000; //nano micro milli second min hour
        bool run = false;        
        virtual void process();
        virtual void _onMainThreadStart();
        virtual void _onMainThreadStopping();
        virtual void _onMainThreadStop();

  public:
        withSingleThread();
        virtual ~withSingleThread();
        void setThreadSleepTimeMicro(int);
        void setThreadSleepTimeMillis(int);
        void start(const char *name);
        void stop();
        std::thread *getProcessThread();
};


#endif