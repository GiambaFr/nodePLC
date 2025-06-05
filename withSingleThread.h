#ifndef SINGLETHREADCONTAINER_H
#define SINGLETHREADCONTAINER_H

#include <thread>
#include <atomic>

class withSingleThread {
  private:  
        std::thread main_thread;
        std::thread processThread();
  protected:
        int sleepTimeMicro = 500*1000; //nano micro milli second min hour
        std::atomic<bool> run = false;        
        virtual void process();
        virtual void _onMainThreadStart();
        virtual void _onMainThreadStopping();
        virtual void _onMainThreadStop();

  public:
        withSingleThread();
        virtual ~withSingleThread();
        void setThreadSleepTimeMicro(int);
        void setThreadSleepTimeMillis(int);
        void start();
        void stop();
        std::thread *getProcessThread();
};


#endif