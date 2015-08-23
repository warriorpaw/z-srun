/*
 * test.c
 *
 *  Created on: Oct 9, 2014
 *      Author: paw
 */


#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/msg.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define printf(x,...) ;

int getLocalInfo(char *);

int sock = -1;

void sig_handle(int sig)
{
	if(sig == SIGINT)
	{
		if(sock != -1)
			close(sock);
		exit(0);
	}
}

int main(int argc,char *args[])
{
	char mac[50];
	char name[50];
	int ver = 117;
	int ret = 0;
	char buf[200];
	struct sockaddr_in servaddr;
	struct sockaddr_in addr;
	socklen_t addr_len;
	int timeout_cont;
	int UDPflag;

	struct timeval tv;
	tv.tv_sec = 50;
	tv.tv_usec = 0;

	addr_len = sizeof(struct sockaddr_in);

	signal(SIGINT,sig_handle);
	if(argc == 2)
	{
		strcpy(name,args[1]);
	}
	else 
		return 0;
	memset(mac,0,50);
	ret = 0;
	while(!mac[0] && ret++ < 10)
	{
		printf("Try to get mac %d times \n",ret);
		getLocalInfo(mac);
		sleep(2);
	}
	if(!mac[0])
	{
		printf("get mac error \n");
		return 0;
	}
	printf("UDP start \n");

    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
    	printf("socket error \n");
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13335);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
    	printf("bind error \n");
        exit(0);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
    {
    	printf("socket error \n");
    	exit(0);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(3338);
    servaddr.sin_addr.s_addr = inet_addr("172.16.108.13");;

	sleep(10);
	UDPflag = 0;
	timeout_cont = 0;
	printf("sleep done !\n");
	while (1)
	{
		memset(buf,0,200);
		if (UDPflag)
		{
			strcpy(buf, name);
		}
		else
		{
			strcpy(buf, "CHECK_VERSION");
			*(int *)(buf+52) = ver;
		}
		if (mac[0])
		{
			strcpy(buf + 32, mac);
		}
		printf("befor send %d!1111\n",sock);
		sendto(sock, buf, 56, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		printf("send done!\n");
		ret = recvfrom(sock,buf,sizeof(buf), 0 , (struct sockaddr *)&addr ,&addr_len);
		if(ret < 0)
		{
			timeout_cont ++;
			printf("recvfrom time out\n");
			if(timeout_cont > 5)
			{
				if(sock != -1)
					close(sock);
				sock = -1;
				system("/etc/init.d/network reload");
				exit(0);
			}
		}
		else
		{
			timeout_cont = 0;
			printf("recvfrom OK %d\n",ret);
			sleep(50);
		}
		UDPflag = 1;
	}
	return 0;
}


int getLocalInfo(char *mac)
{
   int fd;
   int interfaceNum = 0;
   struct ifreq buf[16];
   struct ifconf ifc;
   struct ifreq ifrcopy;

   if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      perror("socket");
      close(fd);
      return -1;
   }

   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = (caddr_t)buf;
   if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
   {
      interfaceNum = ifc.ifc_len / sizeof(struct ifreq);

      while (interfaceNum-- > 0)
      {

    	  if(!strstr(buf[interfaceNum].ifr_name,"ppp"))
    		  continue;
    	  printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);
             ifrcopy = buf[interfaceNum];
             if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
             {
            	 printf("ioctl error \n");
                close(fd);
                return -1;
             }

             //get the mac of this interface
             if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum])))
             {
                snprintf(mac, 50, "%02x:%02x:%02x:%02x:%02x:%02x",
                            (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                            (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                            (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
                            (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                            (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                            (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                printf("device mac: %s\n", mac);
             }
            else
            {
            	printf("ioctl error \n");
                close(fd);
                return -1;
            }
        }
    }
    else
    {
        printf("ioctl error \n");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

