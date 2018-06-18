#include <stdio.h>
#include <stdlib.h>          // necessary libraries.
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/if.h>
#include <linux/if_tun.h>

const char *addr = "/dev/net/tun";
int max(int a , int b){
	return a>b?a:b;
}

int tun_alloc(){
	struct ifreq,ifr;
	int tunfd ,e;

	if((tunfd = open(addr,0_RDWR))<0){
		printf("Cannot open /dev/net/tun\n");
		exit(0);
	}

	memset(&ifr ,0,sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	strncpy(ifr.ifr_name,"tun0",IFNAMSIZ);

	if((e= ioctl(tunfd,TUNSETIFF,(void *)&ifr))<0){
		printf("ioctl() error\n");
		exit(0);
	}

	return tunfd;
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
 	int tapfd , option;
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

 	


 	return 0;
 }