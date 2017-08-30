#!/bin/bash -x

mbed config root .
mbed toolchain GCC_ARM
# find and add missing libraries
mbed deploy

mbed target UBRIDGE

mbed compile -c
