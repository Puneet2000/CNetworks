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

const char *protocol1 ="tcp";
const char *protocol2 = "udp";
const char *protocol3 ="stcp";

unsigned short csum(unsigned short *ptr,int nbytes) 
{
    register long sum;
    unsigned short oddbyte;
    register short answer;
 
    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }
 
    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
     
    return(answer);
}

void my_ip ( char * buffer)
{
    int sock = socket ( AF_INET, SOCK_DGRAM, 0);
 
    const char* kGoogleDnsIp = "8.8.8.8";
    int dns_port = 53;
 
    struct sockaddr_in serv;
 
    memset( &serv, 0, sizeof(serv) );
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons( dns_port );
 
    int err = connect( sock , (const struct sockaddr*) &serv , sizeof(serv) );
 
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*) &name, &namelen);
 
    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);
 
    close(sock);
}

void send_udp_packet(int sockfd , int port , struct hostent* he){
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


int receive_udp_packet(int recvfd){

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
    		send_udp_packet(sendfd, port, he);

    		if(receive_udp_packet(recvfd) == 1)
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
  else if(strcmp(argv[2],protocol3)==0){

    if((sockfd = socket (AF_INET, SOCK_RAW , IPPROTO_TCP))<0){
      printf("socket creation failed..\n");
      return 1;
    }

    char datagram[4096];  
    int source_port = 43591;
    char source_ip[20];
    my_ip( source_ip );  
    struct iphdr *iph = (struct iphdr *) datagram;
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));

    struct sockarddr_in dest;

    bzero(&dest,sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr = *((struct in_addr *)he->h_addr);

    memset (datagram, 0, 4096); 

    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct ip) + sizeof (struct tcphdr);
    iph->id = htons (54321);
    iph->frag_off = htons(16384);
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;   
    iph->saddr = inet_addr ( source_ip );   
    iph->daddr = dest.sin_addr;
    iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
  
    tcph->source = htons ( source_port );
    tcph->dest = htons (80);
    tcph->seq = htonl(1105024978);
    tcph->ack_seq = 0;
    tcph->doff = sizeof(struct tcphdr) / 4; 
    tcph->fin=0;
    tcph->syn=1;
    tcph->rst=0;
    tcph->psh=0;
    tcph->ack=0;
    tcph->urg=0;
    tcph->window = htons ( 14600 ); 
    tcph->check = 0; 
    tcph->urg_ptr = 0;

    for(port = portLow ; port<=portHigh ;port++){

      tcph->dest = htons ( port );
      tcph->check = 0; 

         

    }


     

  }
  else {
    printf("invalid protocol type reffered (udp/tcp expected)..\n")
  }

return 0;
}
