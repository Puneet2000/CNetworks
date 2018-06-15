#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#include	<netinet/in.h>
#include  <netinet/ip.h>
#include  <netinet/ip_icmp.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>	
#include<time.h>

const char *protocol1 ="tcp";
const char *protocol2 = "udp";


void send_packet(int sockfd , int port , struct hostent* he){
  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  char *buf = "This is udp port scanning";
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  servaddr.sin_addr = *((struct in_addr *)he->h_addr);

  if(sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
  {
    printf("cant send message buffer");
  }
}

int receive_packet(int recvfd){
	struct ip *iphdr = malloc(sizeof(struct ip));
   struct timeval timeinterval;
   timeinterval.tv_sec =1;
   timeinterval.tv_usec =0;
   u_char iplen;
   struct icmp *icmp = malloc(sizeof(struct icmp));
   char buf[4096];
   fd_set fds;
   while(1){
   	FD_ZERO(&fds);
    FD_SET(recvfd, &fds);

    if(select(recvfd + 1, &fds, NULL, NULL, &timeinterval) > 0)
    {
      recvfrom(recvfd, &buf, sizeof(buf), 0x0, NULL, NULL);
    }
    else if(!FD_ISSET(recvfd, &fds))
      return 1;
    else
      perror("*** recvfrom() failed ***");

    iphdr = (struct ip *)buf;
    iplen = iphdr->ip_hl << 2;
		  
    icmp = (struct icmp *)(buf + iplen);

    if((icmp->icmp_type == ICMP_UNREACH) && (icmp->icmp_code == ICMP_UNREACH_PORT))
      return 0;
   }
}

int main(int argc , char *argv[]){

	if(argc<5){
		printf("arguments supplied are less (expected 4)\n");
		return 1;
	}else if (argc>5){
		printf("arguments supplied are more (expected 4)\n");
		return 1;
	}

	int portLow = atoi(argv[3]);
	int portHigh = atoi(argv[4]);
	if(portLow <1){
		printf("port %d is not a valid port ( must be > 0)..\n",portLow);
		return 1;
	}

	if(portHigh>65535){
		printf("port %d is not a valid port ( must be <= 65535)..\n",portHigh);
		return 1;
	}
	struct hostent* he = malloc(sizeof(struct hostent));
	int port=0;
    int sockfd;   
    struct sockaddr_in servaddr;
	struct servent *srvport=malloc(sizeof(struct servent));
	time_t ticks;

    printf("scanning %s %s from port range %d - %d \n",argv[1],argv[2],portLow,portHigh);
	if(strcmp(argv[2],protocol1)!=0 && strcmp(argv[2],protocol2)!=0){
		printf("Invalid protocol name (only tcp or udp allowed )\n");
		return 1;
	}
  if((he = gethostbyname(argv[1])) == NULL)
  {
    printf("Host not found \n");
    exit(-1);
  }
  if(strcmp(argv[2],protocol1)==0){
  	ticks = time(NULL);
  	for(port = portLow ; port <=portHigh ;port++){

  		if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	  {
	    printf("Unable to open socket stream\n");
	    exit(-1);
	  }
	  bzero(&servaddr,sizeof(servaddr));
	  servaddr.sin_family = AF_INET;
      servaddr.sin_port = htons(port);
      servaddr.sin_addr = *((struct in_addr *)he->h_addr);
      if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == 0)
	  {
         srvport = getservbyport(htons(port), protocol1);

	    if(srvport != NULL)
		printf("tport %d: %s\n", port, srvport->s_name);

	    fflush(stdout); 
	  }
	  close(sockfd);


  	}
   printf("\nscanning finished on %.24s\r\n",ctime(&ticks));
  }
  else if (strcmp(argv[2],protocol2)==0){
    ticks = time(NULL);
    int user = getuid();
    if(user!=0)
    {
      printf("\nrunning this scan require root privleges..\n");
      return 1;

    }
 
  	int sendfd,recvfd;
  	if((sendfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	 {
	   printf("socket declaration not formed\n");
	   return 1;
	 }
	 // open receive ICMP socket
	 if((recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
	 {
	   printf("socket declaration failed\n");
	   return 1;
	 }

	 for(port = portLow; port <= portHigh; port++)
  	 {
    		send_packet(sendfd, port, he);

    		if(receive_packet(recvfd) == 1)
    		{
		     srvport = getservbyport(htons(port), protocol2);

		  if (srvport != NULL)
		    printf("tport %d: %s\n", port, srvport->s_name);
	
		  fflush(stdout); 
    		}
  	 }
     close(sendfd);
     close(recvfd);
     printf("\nscanning finished on %.24s\r\n",ctime(&ticks));
  }

return 0;
}
