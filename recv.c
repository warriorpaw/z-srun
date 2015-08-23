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

struct msg_st
{
    long int msg_type;
    int data;
};

char *phase[] =
{
"PHASE_DEAD",
"PHASE_INITIALIZE",
"PHASE_SERIALCONN",
"PHASE_DORMANT",
"PHASE_ESTABLISH",
"PHASE_AUTHENTICATE",
"PHASE_CALLBACK",
"PHASE_NETWORK",
"PHASE_RUNNING",
"PHASE_TERMINATE",
"PHASE_DISCONNECT",
"PHASE_HOLDOFF",
"PHASE_MASTER"
};


void run_msg();
void init_msg();

int msgid = -1;
int runing = 0;
int pppd_pid = -1;
int udp_pid = -1;
char name[50];
char pass[50];
char mac[50];
int _udp = -1;
int ver;
pthread_t tid_udp;
int tryed = 0;
int last = 0;



int main()
{
	init_msg();
	while(1)
	{
		tryed++;
		printf("--------start--------\n++try %d times \n",tryed);
		run_msg();
		printf("++error %d times \n---------end---------\n\n\n",tryed);
	}
	return 0;
}

void init_msg()
{
	//msgctl(msgid,IPC_RMID,NULL);
	msgid = msgget((key_t)1984, 0666 | IPC_CREAT);
	if(msgid == -1)
	{
		printf("msgget 2 error\n");
		exit(0);
	}
	memset(mac, 0, sizeof(mac));
}

void run_msg()
{
	struct msg_st data;
	last = -1;
	while(1)
	{
        if(msgrcv(msgid, (void*)&data, sizeof(struct msg_st), 0, 0) == -1)
        {
            fprintf(stderr, "msgrcv failed with errno: %d\n", errno);
        }
        printf("new_phase  %d -- %s\n",data.data,phase[data.data]);
        if(data.data == 8)
        {
        	getLocalInfo();
        	last = 8;
        	if(mac[0])
        	{
        		tryed = 0;
        	}
        	else
        	{
        		printf("May can't find ppp* \n");
        	}
        }
        if(data.data == 0)
        {
        	//printf("------------------------------------------------------\n");
        	//printf("%d   %d\n",last,data.data);
			if(last == 8)
				printf("---maybe lost  \n");
			if(last == 5)
				printf("---maybe pass error \n");
			if(last < 5)
				printf("---maybe can't find server\n");
			if(last > 5 && last <8)
				printf("---unkonw error\n");
			//printf("------------------------------------------------------\n");
			break;
        }
        if(data.data < 5)
        	if(data.data > last)
        	{
        		//printf("+++%d   %d\n",last,data.data);
        		last = data.data;
        		//printf("+++%d   %d\n",last,data.data);
        	}
        	//last = data.data > last ? data.data : last;
	}
}




int getLocalInfo(void)
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
                snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
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

