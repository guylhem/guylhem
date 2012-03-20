/*
 *  QOTDi v0.1 - Quote Of The Day daemon for inetd
 *  By Guylhem Aznar, http://guylhem.org
 *
 *  Based on tqotdd v0.45 from oferkv@gmail.com
 *
 *  qotd is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2, or (at your option) any
 *  later version.
 *
 *  qotd is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MAX_LINE 512
#define NOT_TODAY "No cookie for you today!\n"
#define QUOTES_FILE "/etc/qotd.txt"

int main(int argc, char *argv[]) {
 char quote[FILENAME_MAX];
 char **quotes_list;
 int total_quotes;
 FILE *qfp;
 time_t  now = time(NULL);
 int d = localtime(& now)->tm_mday;
 int m = localtime(& now)->tm_mon+1;
 int today_i = d+(100*m);

  /* open file */
  if ((qfp = fopen(QUOTES_FILE, "r")) == NULL)
	{
	printf("Failed to open file, terminating...\n");
	exit(EXIT_FAILURE);
	}

  /* count lines in file */
  total_quotes = 0;
  while (fgets(quote, MAX_LINE, qfp) != NULL)
	total_quotes++;
  rewind(qfp);

  /* allocate memory for lines */
  quotes_list = (char **) malloc(total_quotes * sizeof(char *));
  if (quotes_list == NULL)
	{
	printf("Memory allocation for quotes list failed, terminating...\n");
	exit(EXIT_FAILURE);
	}

  /* store quotes per month/day */
  while (fgets(quote, MAX_LINE, qfp) != NULL)
	{
        int monthday=atoi(strndup(quote, 4));
	/* IBM format : quote is prefixed by date as "MMDD "
         * CF: http://www.vm.ibm.com/related/tcpip/publications/multisrv.pdf
         */
	quotes_list[monthday] = strdup(quote + 5);
	if (quotes_list[monthday] == NULL)
	  {
	  printf("Memory allocation for quote failed, terminating...\n");
	  exit(EXIT_FAILURE);
	  }
	}

  /* close file */
  fclose(qfp);

  /* alternative to NOT_TODAY :
   today_i = rand() % total_quotes;
  */

  /* send quote */
  if (quotes_list[today_i]) {
  printf ("%s", quotes_list[today_i]);
  } else {
  printf (NOT_TODAY);
  }

  exit (0);
}
