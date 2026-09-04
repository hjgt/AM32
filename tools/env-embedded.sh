#!/usr/bin/env bash
# Source this to get the embedded toolchain on PATH in the current shell:
#   source tools/env-embedded.sh
TC="$HOME/toolchains"
export PATH="$TC/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi/bin:$TC/xpack-openocd-0.12.0-6/bin:$PATH"
export LD_LIBRARY_PATH="$TC/compatlibs:$LD_LIBRARY_PATH"
export OPENOCD_SCRIPTS="$TC/xpack-openocd-0.12.0-6/openocd/scripts"
echo "[env-embedded] arm-none-eabi-gcc: $(command -v arm-none-eabi-gcc)"
echo "[env-embedded] openocd         : $(command -v openocd)"
echo "Build : make CUSTOM_FD6288_G431 ARM_SDK_PREFIX=arm-none-eabi-"
echo "OCD   : openocd -f tools/openocd-stm32g431-stlink.cfg"
echo "Flash : openocd -f tools/openocd-stm32g431-stlink.cfg -c program_fw -c shutdown"
