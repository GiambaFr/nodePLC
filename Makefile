
CC=gcc
CXX=/usr/bin/g++
RM=rm -f
#CPPFLAGS=-g $(shell root-config --cflags)
#LDFLAGS=-g $(shell root-config --ldflags)
#LDLIBS=$(shell root-config --libs)

LDFLAGS=-g
CPPFLAGS=-std=c++17 -fdiagnostics-color=always -Wno-psabi -g -I/usr/include/
LDLIBS=-lwiringPi -lpthread -lpaho-mqtt3as -lpaho-mqttpp3 -low -lowcapi

SRCS=main.cpp commons.cpp config.cpp bt.cpp digital_input.cpp digital_output.cpp dmx.cpp light.cpp myMQTT.cpp temp_sensors.cpp withConfig.cpp withState.cpp withSingleThread.cpp withMqtt.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: HousePLC

HousePLC: $(OBJS)
	$(CXX) $(LDFLAGS) -o ./build/HousePLC $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend