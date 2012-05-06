/*
 * Copyright (C) Guylhem, 2012
 *
 *   This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <bsd/md5.h>

/* OTP shared secret */
#define OTPSECRET "658736a696bd8b173f70e645520c95ab"
#define OTPPIN_LEN 7
#define OTPCHALLENGE_MAX 32

int main (int argc, char **argv) {
/* MOTP : FIRST 6 chars of MD5(10s granularity EPOCH, 4-7 digit PIN, 16-32 hex SECRET) */
char motp[9 + OTPPIN_LEN + OTPCHALLENGE_MAX + 1];
char *md5full;
int now=time(NULL);
int otpepoch=time(NULL)/10;
MD5_CTX md5;
MD5Init(&md5);

 if (argc < 3) {
  fprintf (stderr, "\nGives a password to reply to the OTP-MD5 challenge\n\nUsage:\t%s shared_secret proposed_pin\n", argv[0]);
  exit(1); 
 }

 /* FIXME: should NULL terminate the args, get the server time using TCP/37
  * etc, but this is just a sample implementation.
  *
  * Meanwhile, what an ugly baby! I won't use that motp-gen :-)
  */

 /* No \n in MOTP */
  sprintf (motp, "%d%s%s", otpepoch, argv[2], argv[1]);

  MD5Update(&md5, motp, strlen(motp));
  md5full = MD5End(&md5, NULL);
  printf ("\nGiven %s\n", motp);
  printf ("\nValid for the next %d seconds:\n\t\t\t\t%.6s\n\n", 10+(otpepoch*10-now), md5full);
  free (md5full);
  exit (0);
}
