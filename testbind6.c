#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

void error(char *msg)
{
  perror(msg);
  exit(0);
}
int main(int argc, char *argv[])
{
   int sock, length, fromlen, n;
   struct sockaddr_in6 server;
   struct sockaddr_in6  from;

   int portNr = 5555;
   char buf[1024];

   length = sizeof (struct sockaddr_in6);

   sock=socket(AF_INET6, SOCK_DGRAM, 0);
   if (sock < 0) error("Opening socket");

   bzero((char *)&server, length);
   server.sin6_family=AF_INET6;
   server.sin6_addr=in6addr_any;
   server.sin6_port=htons(portNr);

   inet_pton( AF_INET6, "::1", (void *)&server.sin6_addr.s6_addr);

   if (bind(sock,(struct sockaddr *)&server,length)<0)
       error("binding");
   fromlen = sizeof(struct sockaddr_in6);
   while (1) {
       n = recvfrom(sock,buf,1024,0,(struct sockaddr *)&from,&fromlen);
       if (n < 0) error("recvfrom");
       write(1,"Received a datagram: ",21);
       write(1,buf,n);
       n = sendto(sock,"Got your message\n",17,
                  0,(struct sockaddr *)&from,fromlen);
       if (n  < 0) error("sendto");
   }
}
