#include <stddef.h>
#include <signal.h>
#include <sys/msg.h>
#include "pppd.h"

char pppd_version[] = VERSION;

static int udp_pid = -1;
static char old_name[MAXNAMELEN];
static int msgid = -1;

struct msg_st
{
    long int msg_type;
    int data;
};

static void change_srun_name(char *name)
{
	char tmp[MAXNAMELEN];
	int i;
	if(strlen(name) > MAXNAMELEN - 9)
	{
		//printf("Can't change username!!\n");
		exit(0);
	}
	memset(old_name,0,MAXNAMELEN);
	strcpy(old_name,name);
	strcpy(tmp,"{SRUN3}\r\n");
	for(i = 0; i < strlen(name);i++)
		tmp[i+9] = name[i] + 4;
	tmp[i+9] = '\0';
	strcpy(name,tmp);
}

void fork_udp()
{
	char *argv[] = {"test", old_name , NULL};
	udp_pid = fork();
	if(udp_pid < 0)
	{
		//printf("fork error\n");
		exit(0);
	}
	if(udp_pid > 0)
	{
		return;
	}
	if(udp_pid == 0)
		execv("/jffs/test",argv);
}

static int my_new_phase_hook(int p)
{
    struct msg_st data;
    data.msg_type = 1;
    data.data = p;
    if(msgsnd(msgid, (void*)&data, sizeof(struct msg_st), IPC_NOWAIT) == -1)
    {
        fprintf(stderr, "msgsnd failed\n");
        //exit(EXIT_FAILURE);
    }
	if(p == 8 && udp_pid < 0)
		fork_udp();
	if(p == 0 && udp_pid > 0)
	{
		kill(udp_pid,SIGINT);
		udp_pid = -1;
	}
	if(user[0] != 0 && user[0] != '{')
		change_srun_name(user);
	return 0;
}

void plugin_init(void)
{
	msgid = msgget((key_t)1984, 0666 | IPC_CREAT);
	if(msgid == -1)
	{
		printf("msgget error\n");
		exit(0);
	}
	info("plugin_init");
	new_phase_hook = my_new_phase_hook;
}
