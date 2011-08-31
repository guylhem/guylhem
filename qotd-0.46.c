/*
 *  QOTD v0.46 - "IBM like" Quote Of The Day daemon
 *  By Guylhem Aznar, http://guylhem.org
 *
 *  Based on tqotdd v0.45 from oferkv@gmail.com
 *
 *  tqotdd is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2, or (at your option) any
 *  later version.
 *
 *  tqotdd is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define MAX_LINE 512
#define PORT "17"
#define BACKLOG 10
#define NOT_TODAY "No cookie for you today!\n"

char quotes_file_name[FILENAME_MAX];
char quote[FILENAME_MAX];
char **quotes_list;
int total_quotes;
char port[16];
int no_daemon;

void daemonize()
  {
  pid_t pid, sid;

  /* new child */
  pid = fork();
  if (pid < 0)
	{
	printf("Failed to create child process\n");
	exit(EXIT_FAILURE);
	}

  if (pid > 0)
	{
	exit(EXIT_SUCCESS);
	}

  /* new session ID */
  sid = setsid();
  if (sid < 0)
	{
	printf("Failed to create new session\n");
	exit(EXIT_FAILURE);
	}

  /* output redirection */
  freopen("/dev/null", "r", stdin);
  freopen("/dev/null", "w", stdout);
  freopen("/dev/null", "w", stderr);
  }

void load_quotes_file()
  {
  /* open file */
  FILE *qfp;
  if ((qfp = fopen(quotes_file_name, "r")) == NULL)
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
  }

void *send_qotd(void *th_cli_sock)
  {
  pthread_detach(pthread_self());
  int rslt;
  time_t  now = time(NULL);
  int d = localtime(& now)->tm_mday;
  int m = localtime(& now)->tm_mon+1;
  int today_i = d+(100*m);

  /* alternative to NOT_TODAY :
   today_i = rand() % total_quotes;
  */

  /* send quote */
  if (quotes_list[today_i]) {
  rslt = send((int) th_cli_sock, quotes_list[today_i], strlen(
	  quotes_list[today_i]), 0);
  } else {
  rslt = send((int) th_cli_sock, NOT_TODAY, strlen(NOT_TODAY), 0);
  }
  if (rslt < 0)
	printf("Error sending quote: %s\n", strerror(errno));

  rslt = close((int) th_cli_sock);
  if (rslt < 0)
	printf("Error closing socket: %s\n", strerror(errno));

  return (NULL);
  }

void server_init()
  {
  int ret;

  /* set address */
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;

  ret = getaddrinfo(NULL, port, &hints, &result);
  if (ret != 0)
	{
	printf("Failed to set server address\n");
	exit(EXIT_FAILURE);
	}

  /* allocate socket */
  socklen_t srv_sock, cli_sock;
  srv_sock
	  = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (srv_sock < 0)
	{
	printf("Failed to allocate socket: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
	}

  /* set SO_REUSEADDR on socket */
  int yes = 1;
  ret = setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (ret < 0)
	{
	printf("Error setting socket options: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
	}

  /* bind  */
  ret = bind(srv_sock, result->ai_addr, result->ai_addrlen);
  if (ret < 0)
	{
	printf("Failed to bind socket: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
	}

  /* listen */
  ret = listen(srv_sock, BACKLOG);
  if (ret < 0)
	{
	printf("Failed to listen on socket: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
	}

  /* server loop */
  struct sockaddr_storage cliaddr;
  socklen_t cliaddr_len;
  cliaddr_len = sizeof(cliaddr);
  pthread_t tid;

  for (;;)
	{
	/* accept */
	if ((cli_sock
		= accept(srv_sock, (struct sockaddr *) &cliaddr, &cliaddr_len)) == -1)
	  {
	  printf("Failed accept on socket: %s\n", strerror(errno));
	  continue;
	  }

	/* treat connection */
	pthread_create(&tid, NULL, &send_qotd, (void *) cli_sock);
	}
  }

int main(int argc, char *argv[])
  {
  /* ignore sigpipe */
  signal(SIGPIPE, SIG_IGN);

  srand((unsigned int) (time(NULL) ^ getpid()) + getppid());
  strcpy(port, PORT);
  no_daemon = 0;

  /* command arguments */
  int opt = 0;

  while ((opt = getopt(argc, argv, "f:p:d")) != -1)
	switch (opt)
	  {
	case 'f':
	  strcpy(quotes_file_name, optarg);
	  break;
	case 'p':
	  strcpy(port, optarg);
	  break;
	case 'd':
	  no_daemon = 1;
	  break;
	default:
	  printf("Usage: %s [-f quotes_file] [-p port] (default: 17)\n", argv[0]);
	  exit(EXIT_FAILURE);
	  }

  if (no_daemon == 0)
	daemonize();
  load_quotes_file();
  server_init();
  return 0;
  }
