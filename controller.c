#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<math.h>
#include<linux/sched.h>
#include<pthread.h>
#include<time.h>
#include<sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include<linux/sched.h>
#include <time.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <errno.h>

// Controller spawns one process per task
 
int main(int argc,char *argv[])
{
	
	FILE *cpuset_t;
	FILE *f;
	FILE *f1;
	FILE *op;
	char wf[100];
	char *line = NULL;
	
	
	char *taskId = NULL;
	

	size_t len = 0;
	size_t len1 = 0;

	ssize_t read;

	int flag=0;
	int prev=0;

	int start_time;
	
	int intr;
	int  pid;
	int scale=1;
	
	char cpu_grp[100];
	
	f = fopen("tasks_scaled/arrival_times.txt", "r");
	f1 = fopen("tasks_scaled/tasks.txt", "r");

	op = fopen("wl_resp.txt","a");
	fprintf(op,"SNo,Task Index, Priority, Start Time, End Time, Start Wall Time, End Wall Time, Runtime,Period,Actual Runtime,Actual Period,Sleep Time, Actual Sleep Time\n");
	fclose(op);

	printf("files opened\n");	
	
	
	while((read = getline(&line, &len, f)) != -1)
	{
		if(!flag)
		{
			getline(&taskId,&len1,f1);
			
			intr = 0;
			prev = start_time;

			start_time = atoi(line)/scale;// scale variable can be used to decrease window for testing purposes 	
			flag=1;
		}
		else
		{
			getline(&taskId,&len1,f1);
			start_time = atoi(line)/scale;
			intr = start_time - prev;
			 sleep(intr);
			
		}

		//spawn a process per task, pass it the task id
		pid = fork();
		
		if(pid==0)
		{
			// printf("spawning %s\n",taskId);
				
			if(execl("./emulate_job",taskId,NULL)==-1)//change
				perror("exec");
			printf("exec failed\n");
			exit(0);
			
		}
		prev = start_time;
	}

	int status;
	int count_lines=0;

	while(1){

		status=wait(-1);
		if(status ==-1 && errno==ECHILD){
			break;
		}

	}
	
	
	printf("\n=======Run completed============\n");
	fclose(f);
	fclose(f1);
	return 0;
}
