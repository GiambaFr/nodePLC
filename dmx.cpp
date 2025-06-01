/*******************************************/
/* Velleman K8062 DMX controller for Linux */
/* Helper functions for VM116/K8062        */
/*                                         */
/* Compile with gcc -o dmxd dmxd.c -lusb   */
/* (c) Denis Moreaux 2008                  */
/* Denis Moreaux <vapula@endor.be>         */
/*                                         */
/*******************************************/

#ifdef DEBUG_DMX
#define LOG(...) do { printf(__VA_ARGS__); printf("\r\n"); fflush(stdout);} while (0)
#else
#define LOG(...)
#endif


#include <iostream>


#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "dmx.h"

int *dmx_maxchannels;
int *dmx_shutdown; //stop driver
int *dmx_caps;
int *dmx_channels;

int *shm;
int shmid;

void dmx_open()
{
    shmid=shmget(SH_ADDRESS, sizeof(int)*522, 0666); //0x56444D58
    shm=(int *)shmat(shmid, NULL, 0);
    dmx_maxchannels = shm;
    dmx_shutdown = shm + 1;
    dmx_caps= shm + 2;
    dmx_channels = shm + 10;
}

void dmx_close()
{
    LOG("DMX closing...");
    shmdt(shm);
    LOG("DMX closed.");
}

void dmx_init() {
    LOG("DMX init...");
    dmx_open();
    *dmx_maxchannels = 4;
    *dmx_shutdown = 0;
    *dmx_caps = 0;
     
    LOG("DMX initialized");
}

void dmx_setChannelValue(int channel, int value) {
    dmx_channels[channel] = value;
}