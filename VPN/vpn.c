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

int max(int a , int b){
	return a>b?a:b;
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

int tun_read(int fd, char *buf , int n){
	int byteread;
	if((byteread = read(fd,buf,n))<0){
		printf("error in reading \n");
		exit(0);
	}
	return byteread;
}

int tun_write(int fd , char *buf , int n){
	int bytewritten;
	if((bytewritten = write(fd,buf,n))<0){
		printf("unable to write data\n");
		exit(0);
	}
	return bytewritten;
}
 int main(int argc , char *argv[]){
 	char interface_name[IFNAMSIZ] = "";
 	int tapfd , option=0;
 	int flags = IFF_TUN;
 	int maxfd;
 	char targetip[20]="";
 	uint16_t byteread, bytewritten , length;
 	char buffer[BUFSIZE];
 	struct sockaddr_in local , target;
 	int port = PORT;
 	int sockfd, netfd , optval=1;
 	socklen_t targetlen;
 	long int tap2net=0 , net2tap=0;
 	int errno;
 	int clientorserver=-1;

 	while ((option = getopt(argc,argv,"i:sc:p:ua"))>0){
 		switch(option){
 			case 'i':
 				strncpy(interface_name,optarg,IFNAMSIZ-1);
 				break;
 			case 's':
 				clientorserver = SERVER;
 				break;
 			case 'c' :
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
 			default :
 				printf("unkown argument encountered %c\n",option);
 				exit(0);

 		}
 	}

 	argv += optind;
    argc -= optind;
    if(argc > 0) {
    printf("Too many options!\n");
    exit(1);
      }

	if(*interface_name =='\0'){
		printf("must specify interface name\n");
		return 1;
	}else if(clientorserver<0){
		printf("must specify client or server mode\n");
		return 1;
	}else if((clientorserver==CLIENT)&& (*targetip=='\0')){
		printf("must specify server address\n");
		return 1;
	}

	if ((tapfd = tun_alloc(interface_name,flags | IFF_NO_PI))<0){
		perror("error creating interface : ");
		exit(0);
	}

	printf("successfully connected to interface %s\n",interface_name);

	if((sockfd==socket(PF_INET,SOCK_STREAM,0))<0){
		perror("socket error : ");
		return 1;
	}

	if(clientorserver==CLIENT){
		memset(&target,0,sizeof(target));
		target.sin_family =AF_INET;
		target.sin_addr.s_addr = inet_addr(targetip);
		target.sin_port = htons(port);

		if(connect(sockfd,(struct sockaddr*)&target,sizeof(target))<0){
			perror("connection error : ");
			return 1;
		}

		netfd = sockfd;
		printf("client connected to server\n");

	}else if (clientorserver==SERVER){
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0){
               perror("setsockopt() :");
                  exit(1);
}

		
		memset(&local,0,sizeof(local));
		local.sin_family = AF_INET;
		local.sin_addr.s_addr = htonl(INADDR_ANY);
		local.sin_port = htons(port);

		if(bind(sockfd ,(struct sockaddr*) &local,sizeof(local))<0){
			perror("bind error : ");
			return 1;
		}

		if(listen(sockfd,5)<0){
			perror("listen error : ");
			return 1;
		}
		targetlen = sizeof(target);
		memset(&target ,0, targetlen);
		if((netfd = accept(sockfd,(struct sockaddr*)&target,&targetlen))<0){
			perror("error in accept() : ");
			exit(1);
		}


	}
	else{
		printf("wrong input \n");
		return  1;
	}
	maxfd = max(tapfd,netfd);

	while(1){
		int ret;
		fd_set rdset;
		FD_ZERO(&rdset);
		FD_SET(tapfd,&rdset);
		FD_SET(netfd,&rdset);

		ret = select(maxfd+1,&rdset,NULL,NULL,NULL);

		if(ret < 0 && errno == EINTR){
			continue;
		}

		if(ret <0){
			printf("error in select\n");
			exit(1);
		}

		if(FD_ISSET(tapfd,&rdset)){

			byteread = tun_read(tapfd,buffer,BUFSIZE);
			tap2net++;

			length = htons(byteread);
			bytewritten = tun_write(netfd,(char *)buffer,sizeof(length));
			byteread =tun_write(netfd,buffer,byteread);

			printf("TAP2NET %lu : written %d bytes to the network\n" , tap2net,bytewritten);

		}

		if(FD_ISSET(netfd,&rdset)){

			byteread = tun_read(netfd,(char *)&length,sizeof(length));
			if(byteread==0){
				break;
			}

			net2tap++;
			byteread = tun_read(netfd,buffer,ntohs(length));
			printf("NET2TAp %lu : read %d bytes from the network\n" , net2tap,byteread);
			bytewritten = tun_write(tapfd,buffer,byteread);

			printf("NET2TAP %lu : written %d bytes to the network\n" , net2tap,bytewritten);

		}


	}

 	return 0;
 }
