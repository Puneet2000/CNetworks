#include "unp.h"
#include<time.h>

char *protocol1 ="tcp";
char *protocol2 = "udp";

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

return 0;
}