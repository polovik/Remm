#!/bin/bash

rm *.o
g++ $1.c -c
g++ $1.o -o $1 -lpthread -lopencv_core -lopencv_highgui
