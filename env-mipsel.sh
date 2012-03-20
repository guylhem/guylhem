#!/bin/bash
#export PATH=$PATH:/usr/mips-unknown-linux-uclibc/bin
export CC=/usr/bin/mips-linux-gcc
export CFLAGS="-O3 -march=24kec -mips32r2 -mdsp -static -static-libgcc -Wl,-static"
#export CFLAGS="-O3 -march=24kec -mips16 -mdsp -static -static-libgcc -Wl,-static"
export LDFLAGS="-L/usr/mips-unknown-linux-uclibc/lib"
export CPPFLAGS="-I/usr/mips-unknown-linux-uclibc/include" 
export CPP="/usr/bin/mips-linux-cpp"
#export CROSS_COMPILE="mips-linux-"
export ARCH="mips-linux"
export CONFIG="--host i686-unknown-linux-gnu --target mips-linux --enable-static"
/bin/bash
