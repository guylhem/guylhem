/*
 * Copyright (C) Guylhem, 2012
 *
 * Put in libb/ to add MOTP support to busybox /bin/login and anything else
 * that uses correct_password to check whether the password is correct (ex:
 * telnetd)
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

#define ENABLE_FEATURE_LOGINOTP 1

#include "libbb.h"

#if ENABLE_FEATURE_LOGINOTP

/* For a PIN of non-repeating digits :
 *
 * Floyd algorithm (elements of size N, lenght of M)
 * Then a Knuth Shuffle
 */
#define OTPPIN_LEN 6
#define OTPPIN_SIZ 9

/* OTP reply length */
#define OTPCHALLENGE_MAX 6

/* OTP shared secret */
#define OTPSECRET "658736a696bd8b173f70e645520c95ab"

/* OTP acceptable window before/after current time in minutes */
#define OTPWINDOW 2

/* OTP valid iff all chars are hexadecimal */
#define OTPCHARS "0123456789abcdefABCDEF"
/* Pretend to read a password while it's known to be incorrect?  Since we won't
 * accept it, don't bother, even if it might theoretically be used in a
 * carefull measurement of the response delay attack to detect which user *do*
 * exist with a dictionnary-based attack. 
 */
#define PRETEND sprintf (challenge, "Password or OTP-MD5 Challenge %s @ %d: ", pin, otpepoch); unencrypted = bb_ask_stdin(challenge); return (0);
#else
#define PRETEND unencrypted = bb_ask_stdin("Password: "); return (0);
#endif

void pin_floyd(char pin[]) {
int in, im;
int list[OTPPIN_LEN];
im = 0;

/* seed from epoch and salt with shared secret 1st decimal segment len */
srand((int) (time(NULL) / 1+strspn(OTPSECRET, "0123456789")));

for (in = 0; in < OTPPIN_SIZ && im < OTPPIN_LEN; ++in) {
  int rn = OTPPIN_SIZ - in;
  int rm = OTPPIN_LEN - im;
  if (rand() % rn < rm)    
    list[im++] = in + 1;
}

for (in = OTPPIN_LEN-1; in >1; --in) {
    int out = rand() % in + 1;
    int temp = list[out];
    list[out] = list[in];
    list[in] = temp;
}

for (in = 0; in < OTPPIN_LEN; ++in) {
  sprintf (&pin[in], "%1d", list[in]);
}

 pin[OTPPIN_LEN]='\0';

}

/* correct_password : ask the user its password.
 *
 * in: const struct passwd *pw : NULL if inexistant user tries to login
 * out: 1 if correct password || empty password
 *      0 if wrong password
 */

int FAST_FUNC correct_password (const struct passwd *pw) {
        char *unencrypted, *encrypted;
        const char *correct;
	int i;
#if ENABLE_FEATURE_SHADOWPASSWDS
        /* Using _r function to avoid pulling in static buffers */
        struct spwd spw;
        char buffer[256];
#endif
#if ENABLE_FEATURE_LOGINOTP
	char challenge[255];
	char pin[OTPPIN_LEN+1];
	/* MOTP : FIRST 6 chars of MD5(10s granularity EPOCH, 4-7 digit PIN, 16-32 hex SECRET) */
	char motp[9 + OTPPIN_LEN + 32];
  	uint32_t md5[16/4];
        // char *md5;
	/* 12 series of 5 seconds per minutes window in the past and the future */
	char acceptable[2*OTPWINDOW*12][6+1];
	/* initiate epoch at the window past to ease array creation*/
	int otpepoch=time(NULL)/10 - OTPWINDOW*12;

 /* get ready if PRETENDing is necessary */
  pin_floyd(pin);

#endif

/* empty password for non existing users */
if (!pw) {
PRETEND
}

	correct=pw->pw_passwd;

#if ENABLE_FEATURE_SHADOWPASSWDS
        if ((correct[0] == 'x' || correct[0] == '*') && !correct[1]) {
                struct spwd *result = NULL;
                i = getspnam_r(pw->pw_name, &spw, buffer, sizeof(buffer), &result);
                /* glibc 2.4 BUG: getspnam_r returns 0 instead of -1, but set result to NULL */
  	        if (i || !result) {
	 	  PRETEND
		} else {
		  correct = result->sp_pwdp;
		}
        }
#endif

/* If the password field is really empty, accept it without a prompt */
        if (!correct[0]) {
                return (1);
	}

#if ENABLE_FEATURE_LOGINOTP
  sprintf (challenge, "Password or OTP-MD5 Challenge %s @ %d: ", pin, otpepoch);
  unencrypted = bb_ask_stdin(challenge);
#else
        unencrypted = bb_ask_stdin("Password: ");
#endif
        if (!unencrypted) {
                return (0);
        }
        encrypted = pw_encrypt(unencrypted, correct, 1);
        i = (strcmp(encrypted, correct) == 0);
        free(encrypted);

#ifndef ENABLE_FEATURE_LOGINOTP
        memset(unencrypted, 0, strlen(unencrypted));
        return (i);
#else
  /* First try it as a password */
   if (i==1) {
        memset(unencrypted, 0, strlen(unencrypted));
	return (1);
   }

   /* Now that we know it doesn't match, better be safe than sorry */

    /* pad with spaces at the exact len to avoid early strstr matches.
     * It might also avoid some potential unicode strstr weirdness
     */
    sprintf (unencrypted, "%-*.*s", OTPCHALLENGE_MAX, OTPCHALLENGE_MAX, unencrypted);

  /* now create a list of the valid answer to the challenge given the time window */
  for (i=0; i<2*OTPWINDOW*12; ++i) {
  md5_ctx_t ctx;

  /* No \n in MOTP or your md5 won't match */
  sprintf (motp, "%d%s%s", otpepoch+i, pin, OTPSECRET);

  /* Do each md5 */
  md5_begin(&ctx);
  md5_hash(&ctx, motp, strlen(motp));
  // md5=xmalloc(4096);
  md5_end(&ctx, md5);

  /* get first 6 hex chars and add a space*/
   sprintf (acceptable[i], "%02x", md5[0]);
   sprintf (acceptable[i]+2, "%02x", md5[1]);
   sprintf (acceptable[i]+4, "%02x", md5[2]);
   sprintf (acceptable[i]+6, " ");

  // free (md5);
  }

  if (strcasestr((char *) acceptable, unencrypted)) {
    memset(unencrypted, 0, strlen(unencrypted));
    return (1);
  } else {
    memset(unencrypted, 0, strlen(unencrypted));
    return (0);
  }
#endif /* ENABLE_FEATURE_LOGINOTP */

}
