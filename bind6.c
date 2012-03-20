/*
   Copyright (C) 2012 Guylhem Aznar <code@guylhem> http://www.guylhem.net/
   Copyright (C) 2000 Daniel Ryde <daniel@ryde.net>  http://www.ryde.net/

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   LD_PRELOAD library to make bind and connect to use a virtual
   IP address as localaddress. Specified via the enviroment
   variable BIND_IPVx.

   Compile on Linux with:
   	gcc -nostartfiles -fpic -shared bind6.c -o bind6.so -ldl -D_GNU_SOURCE

   Example to make inetd only listen to the 127.0.0.1, thus disabling remote
   connections and only enable to/from localhost:
   	BIND_IPV4="127.0.0.1" LD_PRELOAD=./bind.so /sbin/inetd
   With a IPV4 like IPV6:
   	BIND_IPV6="::ffff:127.0.0.1" LD_PRELOAD=./bind.so /sbin/inetd

   To restrict inetd to a given IPV6:
   	BIND_IPV6="2001:db8::1" LD_PRELOAD=./bind.so /sbin/inetd
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dlfcn.h>
#include <errno.h>

int (*real_bind)(int, const struct sockaddr *, socklen_t);
int (*real_connect)(int, const struct sockaddr *, socklen_t);

char *bind_addr_env;
struct sockaddr_in local_sockaddr_in[] = { 0 };

char *bind_addr6_env;
struct sockaddr_in6 local_sockaddr_in6[] = { 0 };

char addressBuffer[INET6_ADDRSTRLEN];
#define in6addr_any_saddr IN6ADDR_ANY_INIT;

void
_init (void)
{
    const char *err;
    void    *uclibc;

    uclibc = dlopen("/lib/libc.so.0", RTLD_LOCAL | RTLD_LAZY);
    // glibc can do with
    // real_bind = dlsym (RTLD_NEXT, "bind");
    real_bind = dlsym (uclibc , "bind");
    if ((err = dlerror ()) != NULL) {
        fprintf (stderr, "dlsym (bind): %s\n", err);
    }

    if (bind_addr_env = getenv ("BIND_IPV4")) {
        local_sockaddr_in->sin_family = AF_INET;
        local_sockaddr_in->sin_port = htons (0);
    // old IPV4:
        // local_sockaddr_in->sin_addr.s_addr = inet_addr (bind_addr_env);
    // new IPV4:
        inet_pton(AF_INET, bind_addr_env, &local_sockaddr_in->sin_addr);
    } else {
	printf ("Please export BIND_IPV4\n");
    }

    if (bind_addr6_env = getenv ("BIND_IPV6")) {
        local_sockaddr_in6->sin6_family = AF_INET6;
        local_sockaddr_in6->sin6_port = htons (0);
        local_sockaddr_in6->sin6_flowinfo = 0;
        inet_pton(AF_INET6, bind_addr6_env, &local_sockaddr_in6->sin6_addr);
    } else {
	printf ("Please export BIND_IPV6\n");
    }
}

int bind (int fd, const struct sockaddr *sk, socklen_t sl)
{
    static struct sockaddr_in *lsk_in;
    static struct sockaddr_in6 *lsk_in6;
    lsk_in = (struct sockaddr_in *)sk;
    lsk_in6 = (struct sockaddr_in6 *)sk;
    static char addr_buf[INET6_ADDRSTRLEN];

    if (lsk_in->sin_family == AF_INET) {
        inet_ntop(AF_INET, &lsk_in->sin_addr, addr_buf, sizeof(lsk_in->sin_addr));
        printf("bind(): AF_INET fd=%d %s:%d\n", fd, addr_buf, ntohs (lsk_in->sin_port));
        if ((bind_addr_env) && (lsk_in->sin_addr.s_addr == INADDR_ANY )) {
            lsk_in->sin_addr.s_addr = local_sockaddr_in->sin_addr.s_addr;
        // debug
            inet_ntop(AF_INET, &lsk_in->sin_addr, addr_buf, sizeof(lsk_in->sin_addr));
            printf("BIND(): fd=%d %s:%d\n", fd, addr_buf, ntohs (lsk_in->sin_port));
        }
    }
    else if (lsk_in6->sin6_family == AF_INET6) {
	void *p = &lsk_in6->sin6_addr.s6_addr;

	if ((bind_addr6_env) {
		inet_ntop(AF_INET6, p, addressBuffer, INET6_ADDRSTRLEN);
//		printf("bind(): wanted %s\n", addressBuffer);
		int err = inet_pton(sk->sa_family, bind_addr6_env, p);
	if (err == 1)
//		printf("bind(): got  %s\n", bind_addr6_env);
	}

//struct in6_addr s6 = { };
//if (!IN6_IS_ADDR_UNSPECIFIED(&s6))
//  inet_pton(AF_INET6, "2001:db8::1", &s6);

    }
    else {
        printf("bind(): af=%d fd=%d\n", fd, lsk_in->sin_family);
    }

    return real_bind (fd, sk, sl);
}

int connect (int fd, const struct sockaddr *sk, socklen_t sl)
{
    static struct sockaddr_in *rsk_in;
    static struct sockaddr_in6 *rsk_in6;
    rsk_in = (struct sockaddr_in *)sk;
    rsk_in6 = (struct sockaddr_in6 *)sk;
    static char addr_buf[INET6_ADDRSTRLEN];

    if (rsk_in->sin_family == AF_INET) {
        inet_ntop(AF_INET, &rsk_in->sin_addr, addr_buf, sizeof(rsk_in->sin_addr));
        printf("connect(): AF_INET fd=%d %s:%d\n", fd, addr_buf, ntohs (rsk_in->sin_port));
        if (bind_addr_env) {
            rsk_in->sin_addr.s_addr = local_sockaddr_in->sin_addr.s_addr;
        // debug
            inet_ntop(AF_INET, &rsk_in->sin_addr, addr_buf, sizeof(rsk_in->sin_addr));
            printf("CONNECT(): fd=%d %s:%d\n", fd, addr_buf, ntohs (rsk_in->sin_port));
	    real_bind (fd, (struct sockaddr *)local_sockaddr_in, sizeof (struct sockaddr));
        }
    }
    else if (rsk_in6->sin6_family == AF_INET6) {
        inet_ntop(AF_INET6, &rsk_in6->sin6_addr, addr_buf, sizeof(rsk_in6->sin6_addr));
        printf("connect(): AF_INET6 fd=%d [%s]:%d\n", fd, addr_buf, ntohs (rsk_in6->sin6_port));
        if (bind_addr6_env) {
            rsk_in6->sin6_addr = local_sockaddr_in6->sin6_addr;
        // debug
            inet_ntop(AF_INET6, &rsk_in6->sin6_addr, addr_buf, sizeof(rsk_in6->sin6_addr));
            printf("CONNECT(): AF_INET6 fd=%d [%s]:%d\n", fd, addr_buf, ntohs (rsk_in6->sin6_port));
	    real_bind (fd, (struct sockaddr *)local_sockaddr_in6, sizeof (struct sockaddr));
        }

//struct in6_addr s6 = { };
//if (!IN6_IS_ADDR_UNSPECIFIED(&s6))
//  inet_pton(AF_INET6, "2001:db8::1", &s6);

    }
    else {
        printf("connect(): af=%d fd=%d\n", fd, rsk_in->sin_family);
    }

    return real_connect (fd, sk, sl);
}

