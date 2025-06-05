/*******************************************/
/* Velleman K8062 DMX controller for Linux */
/* Helper functions for VM116/K8062        */
/*                                         */
/* Compile with gcc -o dmxd dmxd.c -lusb   */
/* (c) Denis Moreaux 2008                  */
/* Denis Moreaux <vapula@endor.be>         */
/*                                         */
/*******************************************/
#pragma once
#ifndef DMX_H
#define DMX_H

#define SH_ADDRESS 0x56444D58

extern int *dmx_maxchannels;
extern int *dmx_shutdown;
extern int *dmx_caps;
extern int *dmx_channels;

void dmx_open();
void dmx_close();

void dmx_init();
void dmx_setChannelValue(int channel, int value);

#endif