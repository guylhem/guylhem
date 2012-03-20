#!/bin/bash
export PATH=/opt/arm-2007q3/bin/:$PATH
export CC=/opt/arm-2007q3/bin/arm-none-linux-gnueabi-gcc
##export CFLAGS="-O2 -fomit-frame-pointer -ffast-math -mlittle-endian -mfloat-abi=softfp -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=vfpv3 -march=armv7-a-neon"
#export CFLAGS="-O2 -fomit-frame-pointer -ffast-math -mlittle-endian -mfloat-abi=softfp -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -march=armv7a"
export CFLAGS="-O2 -mfloat-abi=softfp -march=armv7a"

export LDFLAGS="-L/opt/arm-2007q3/arm-none-linux-gnueabi/lib"
export CPPFLAGS="-I/opt/arm-2007q3/arm-none-linux-gnueabi/include" 
export CPP=/opt/arm-2007q3/bin/arm-none-linux-gnueabi-cpp
#export "CROSS_COMPILE=arm-none-linux-gnueabi-"
export ARCH="arm-none-linux-gnueabi"
export CONFIG="--host i686-unknown-linux-gnu --target arm-none-linux-gnueabi"
#--enable-static"
/bin/bash
