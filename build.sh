#!/bin/bash

PROGRAM=$1
BUILD_FOLDER=build/
GENERATED_UF2_FILE=$BUILD_FOLDER/$PROGRAM.uf2

# Print setup args
echo ------------------------------
echo PROGRAM: $PROGRAM
echo ------------------------------

# Build
cd my-workspace
west build -p always -b dongle_nrf52840 ../$PROGRAM/

# Copy generated file
cd ..
mkdir build
cp my-workspace/build/zephyr/zephyr.uf2 $GENERATED_UF2_FILE

# Print recap
echo ------------------------------
echo Generated .uf2 files in $GENERATED_UF2_FILE
echo ------------------------------
