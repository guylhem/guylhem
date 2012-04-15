mips-linux-gcc -O3 -march=24kec -mips32r2 -mdsp -static -static-libgcc -Wl,-static \
v4l2grab.c -o v4l2grab  \
-I/usr/mips-unknown-linux-uclibc/include -I./jpeg-8d/ \
-L/usr/mips-unknown-linux-uclibc/lib -L./jpeg-8d/.libs \
-ljpeg -DIO_READ -DIO_MMAP -DIO_USERPTR

