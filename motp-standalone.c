/*
 * Copyright (C) Guylhem, 2012
 *
 * Test your MOTP with this standalone version, which depends on BSD libs for
 * md5 functions.
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

/* Floyd algorith for a PIN : elements of size N, lenght of M, followed by a Knuth Shuffle*/
#define OTPPIN_LEN 6
#define OTPPIN_SIZ 9

/* OTP reply length */
#define OTPCHALLENGE_MAX 6

/* OTP shared secret */
#define OTPSECRET "658736a696bd8b173f70e645520c95ab"

/* OTP acceptable window in the future in minutes */
#define OTPWINDOW 3

void
pin_floyd (char pin[])
{
  int in, im;
  int list[OTPPIN_LEN];
  im = 0;

/* seed from epoch - but we are not expecting multiple login per second */
  srand (time (NULL));

  for (in = 0; in < OTPPIN_SIZ && im < OTPPIN_LEN; ++in)
    {
      int rn = OTPPIN_SIZ - in;
      int rm = OTPPIN_LEN - im;
      if (rand () % rn < rm)
	list[im++] = in + 1;
    }

  for (in = OTPPIN_LEN - 1; in > 1; --in)
    {
      int out = rand () % in + 1;
      int temp = list[out];
      list[out] = list[in];
      list[in] = temp;
    }

  for (in = 0; in < OTPPIN_LEN; ++in)
    {
      sprintf (&pin[in], "%1d", list[in]);
    }

  pin[OTPPIN_LEN] = '\0';
}

int
main (int argc, char **argv)
{
  int i;
  char ch;
  char reply[OTPCHALLENGE_MAX + 1];
  int char_count;
  char pin[OTPPIN_LEN + 1];
/* MOTP : FIRST 6 chars of MD5(10s granularity EPOCH, 4-7 digit PIN, 16-32 hex SECRET) */
  char motp[9 + OTPPIN_LEN + 32];
  char *md5full;
/* 12 series of 5 seconds per minutes window in the past and the future */
  char acceptable[2 * OTPWINDOW * 12][6 + 1];
  int window;
/* initiate epoch at the window past to ease array creation*/
  int otpepoch = time (NULL) / 10 - OTPWINDOW * 12;

  MD5_CTX md5;
  MD5Init (&md5);

/* Get a proper pin */
  pin_floyd (pin);

  printf ("OTP-MD5 Challenge: %s @ %d\n", pin, otpepoch);
  ch = getchar ();
  char_count = 0;
  while ((ch != '\n') && (char_count < OTPCHALLENGE_MAX))
    {
      /* avoid regexps - keep hex only */
      if (isxdigit (ch))
	{
	  reply[char_count++] = ch;
	  ch = getchar ();
	}
      else
	{
	  break;
	}
    }

  /* pad with spaces to avoid early matches */
  sprintf (reply, "%-6.6s", reply);

  /* null terminate buffer */
  reply[OTPCHALLENGE_MAX + 1] = 0x00;

  for (window = 0; window < 2 * OTPWINDOW * 12; ++window)
    {
      /* No \n in MOTP */
      sprintf (motp, "%d%s%s", otpepoch + window, pin, OTPSECRET);
      MD5Update (&md5, motp, strlen (motp));
      md5full = MD5End (&md5, NULL);
      sprintf (acceptable[window], "%.6s ", md5full);
      free (md5full);
    }

  // printf ("\n:%s:\n", reply);
  // printf ("\n:%s:\n", acceptable);

  if (strcasestr (acceptable, reply))
    {
      printf ("success\n");
      exit (0);
    }
  else
    {
      printf ("failure\n");
      exit (1);
    }
}
