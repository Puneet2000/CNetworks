#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>

#define BUFSIZE 2000   
#define CLIENT 0
#define SERVER 1
#define PORT 55555
#define IP_LEN 60

int pers=0;
int tapfd;

int max(int a , int b){
	return a>b ? a:b;
}


int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);

  return fd;
}

void setperisitent(int fd){

	int owner;
	owner= getuid();
	if(owner !=-1){
		if(ioctl(fd,TUNSETOWNER,owner)<0){
			perror("TUNSETOWNER");
			exit(1);
		}
	}

	if(ioctl(fd,TUNSETPERSIST,1)<0){
		perror("enabling TUNSETPERSIST");
		exit(1);
	}

	printf("set persistent and owned by %d\n",owner);

}
int vpn_read(int fd, char *buf, int n){
  
  int byteread;

  if((byteread=read(fd, buf, n)) < 0){
    perror("read()");
    exit(1);
  }
  return byteread;
}

int vpn_write(int fd, char *buf, int n){
  
  int bytewrite;

  if((bytewrite=write(fd, buf, n)) < 0){
    perror("write()");
    exit(1);
  }
  return bytewrite;
}

int vpn_read_n(int fd, char *buf, int n) {

  int byteread, left = n;

  while(left > 0) {
    if ((byteread = vpn_read(fd, buf, left)) == 0){
      return 0 ;      
    }else {
      left -= byteread;
      buf += byteread;
    }
  }
  return n;  
}

void setip(int fd , char ip[]){
	struct ifreq ifr;
	struct sockaddr_in addr;
	int stat,s;

	memset(&ifr,0,sizeof(ifr));
	memset(&addr,0,sizeof(addr));
	strncpy(ifr.ifr_name,ip,IFNAMSIZ);

	addr.sin_family = AF_INET;
	s = socket(addr.sin_family,SOCK_STREAM,0);
	stat = inet_pton(addr.sin_family,ip,&addr.sin_addr);
	if(stat==0)
		{
			printf("inet_pton() - invalid ip");
			exit(1);
		}
	if(stat ==-1){
		printf("invalid family");
		exit(1);
	}

	if(stat==1);
	else
	{
		printf("inet_pton()");
		exit(1);
	}

	ifr.ifr_addr = *(struct sockaddr *)&addr;
	char buff[BUFSIZE];
	char *foo;
	foo = inet_ntop(AF_INET,&addr.sin_addr,buff,BUFSIZE);
	if(foo ==NULL){
		printf("inet_ntop()");
		exit(1);
	}else{
		printf("main = %s, addr = %s\n",ip, buff);
		 if (ioctl(s, SIOCSIFADDR, (caddr_t) &ifr) == -1)
        perror("ioctl() - SIOCSIFADDR");
        exit(1);
	}

	if (ioctl(s, SIOCGIFFLAGS, &ifr) == -1)
		perror("ioctl() - SIOCGIFFLAGS");
	ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
	if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1)
		perror("ioctl() - SIOCSIFFLAGS");
}

void usage(){
	printf("---------------USAGE------------\n");
	printf("-i <interface name> : define name to your tun/tap inteface\n");
	printf("-s : make current system as server\n");
	printf("-c <server ip> : connect to server\n");
	printf("-p <port no> : connect to this port DEFAULT PORT IS 55555\n");
	printf("-u : create a tap inteface\n");
	printf("-a : create a tun inteface\n");
	printf("-h : USAGE\n");
	exit(1);
}

void onProgramEnd(){

	if(pers==1){
		if(ioctl(tapfd, TUNSETPERSIST, 0) < 0){
      		perror("disabling TUNSETPERSIST");
      		
	}

}

int main(int argc, char *argv[]) {
  
  atexit(onProgramEnd);
  int option;
  int flags = IFF_TUN;
  char if_name[IFNAMSIZ] = "";
  int maxfd;
  char ip[IP_LEN];
  uint16_t byteread, bytewritten, length;
  char buffer[BUFSIZE];
  struct sockaddr_in local, target;
  char targetip[16] = "";           
  unsigned short int port = PORT;
  int sockfd, netfd, optval = 1;
  socklen_t targetlen;
  int clientorserver = -1;    
  unsigned long int tap2net = 0, net2tap = 0;

  while((option = getopt(argc, argv, "i:sc:p:uahd")) > 0) {
    switch(option) {
      case 'i':
        strncpy(if_name,optarg, IFNAMSIZ-1);
        break;
      case 's':
        clientorserver = SERVER;
        break;
      case 'c':
        clientorserver = CLIENT;
        strncpy(targetip,optarg,15);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'u':
        flags = IFF_TUN;
        break;
      case 'a':
        flags = IFF_TAP;
        break;
       case 'h':
       	usage();
       	break;
       case 'd':
       	pers=1;
       	break;
      default:
        printf("Unknown option %c\n", option);
        exit(1);
    }
  }

  argv += optind;
  argc -= optind;

  if(argc > 0) {
    printf("Too many options!\n");
    exit(1);

  }

  if(*if_name == '\0') {
    printf("Must specify interface name!\n");
    exit(1);
  } else if(clientorserver < 0) {
    printf("Must specify client or server name!\n");
    exit(1);
  } else if((clientorserver == CLIENT)&&(*targetip == '\0')) {
    printf("Must specify itarget ip\n");
    exit(1);
  }

  if ( (tapfd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
    printf("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }
 
  
printf("Successfully connected to interface %s\n", if_name);
setperisitent(tapfd);
// setip(tapfd,ip);


  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(1);
  }

  if(clientorserver == CLIENT) {
    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(targetip);
    target.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*) &target, sizeof(target)) < 0) {
      perror("connect()");
      exit(1);
    }

    netfd = sockfd;
    printf("CLIENT: Connected to server %s\n", inet_ntoa(target.sin_addr));
    
  } else {

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
      perror("setsockopt()");
      exit(1);
    }
    
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr*) &local, sizeof(local)) < 0) {
      perror("bind()");
      exit(1);
    }
    
    if (listen(sockfd, 5) < 0) {
      perror("listen()");
      exit(1);
    }
    targetlen = sizeof(target);
    memset(&target, 0, targetlen);
    if ((netfd = accept(sockfd, (struct sockaddr*)&target, &targetlen)) < 0) {
      perror("accept()");
      exit(1);
    }

    printf("SERVER: Client connected from %s\n", inet_ntoa(target.sin_addr));
  }

  maxfd =max(tapfd,netfd);

  while(1) {
    int ret;
    fd_set rdset;

    FD_ZERO(&rdset);
    FD_SET(tapfd, &rdset); FD_SET(netfd, &rdset);

    ret = select(maxfd + 1, &rdset, NULL, NULL, NULL);

    if (ret < 0 && errno == EINTR){
      continue;
    }

    if (ret < 0) {
      perror("select()");
      exit(1);
    }

    if(FD_ISSET(tapfd, &rdset)) {
     
      
      byteread = vpn_read(tapfd, buffer, BUFSIZE);

      tap2net++;
      printf("TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, byteread);

      length = htons(byteread);
      bytewritten = vpn_write(netfd, (char *)&length, sizeof(length));
      bytewritten = vpn_write(netfd, buffer, byteread);
      
      printf("TAP2NET %lu: Written %d bytes to the network\n", tap2net, bytewritten);
    }

    if(FD_ISSET(netfd, &rdset)) {     
      byteread = vpn_read_n(netfd, (char *)&length, sizeof(length));
      if(byteread == 0) {
        break;
      }

      net2tap++;

      byteread = vpn_read_n(netfd, buffer, ntohs(length));
      printf("NET2TAP %lu: Read %d bytes from the network\n", net2tap, byteread);
      bytewritten = vpn_write(tapfd, buffer, byteread);
      printf("NET2TAP %lu: Written %d bytes to the tap interface\n", net2tap, bytewritten);
    }
  }
  
  return(0);
}