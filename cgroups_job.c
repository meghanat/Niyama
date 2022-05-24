#include<stdio.h>
#include<time.h>
#include<sys/time.h>
#include<stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include<fcntl.h>
#include<linux/sched.h>
#include <string.h>
#include <time.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

// implements algorithm independent of utilization, we sleep instead of performing IO operations because of the intuition that it will interfere with the experiment with CPU contention 

// input parameters : cpu_utilization, cpu_time, taskID, IO time, argv[4] 

float slice = 0.05;
const float scale =1;
const float test_scale=1;
double get_wall_time()
{
	struct timeval time;
	if(gettimeofday(&time,NULL))
	{
		return 0;
	}
	return (double)time.tv_sec + (double)time.tv_usec*0.000001;
}

 typedef struct{

 	int sno;
	char * taskID;
	float start_time;
	float end_time;
	float cpu_time;
	int num_splits;
	int priority;

}task;

#define gettid() syscall(__NR_gettid)

 #define SCHED_DEADLINE	6

 /* XXX use the proper syscall numbers */
 #ifdef __x86_64__
 #define __NR_sched_setattr		314
 #define __NR_sched_getattr		315
 #endif

 #ifdef __i386__
 #define __NR_sched_setattr		351
 #define __NR_sched_getattr		352
 #endif

 #ifdef __arm__
 #define __NR_sched_setattr		380
 #define __NR_sched_getattr		381
 #endif

 static volatile int done;

 struct sched_attr {
	__u32 size;

	__u32 sched_policy;
	__u64 sched_flags;

	/* SCHED_NORMAL, SCHED_BATCH */
	__s32 sched_nice;

	/* SCHED_FIFO, SCHED_RR */
	__u32 sched_priority;

	/* SCHED_DEADLINE (nsec) */
	__u64 sched_runtime;
	__u64 sched_deadline;
	__u64 sched_period;
 };

 int sched_setattr(pid_t pid,
		  const struct sched_attr *attr,
		  unsigned int flags)
 {
	return syscall(__NR_sched_setattr, pid, attr, flags);
 }

 int sched_getattr(pid_t pid,
		  struct sched_attr *attr,
		  unsigned int size,
		  unsigned int flags)
 {
	return syscall(__NR_sched_getattr, pid, attr, size, flags);
 }

void printSchedType()
{
        int schedType;

        schedType = sched_getscheduler(getpid());
        switch(schedType)
        {
                case SCHED_FIFO:
                printf("Pthread Policy is SCHED_FIFO : %d \n",schedType);
                break;
                case SCHED_OTHER:
                printf("Pthread Policy is SCHED_OTHER : %d \n",schedType);
                break;
                case SCHED_RR:
                printf("Pthread Policy is SCHED_RR : %d \n",schedType);
                break;
               	case SCHED_DEADLINE:
                printf("Pthread Policy is SCHED_DEADLINE : %d \n",schedType);
                break;
                default:
                printf("Pthread Policy is UNKNOWN : %d \n",schedType);

        }
}

//used for setting scheduler parameters for SCHED_DEADLINE
int schedule_job(float runtime, float period, float deadline){
	
		struct sched_attr attr;
		unsigned int flags = 0;

		attr.size = sizeof(attr);
		attr.sched_flags = 0;
		attr.sched_nice = 0;
		attr.sched_priority = 0;
		
		attr.sched_policy = SCHED_DEADLINE;
		attr.sched_runtime = runtime*1000*1000*1000*1.1;
		attr.sched_period = period*1000.0*1000.0*1000.0*1.1;
		attr.sched_deadline=deadline*1000*1000*1000*1.1;		
		return sched_setattr(0, &attr, flags);

}

//thread to log cpu utilisation of cpu thread
void * log_cpu_usage(void * args){
	double t_start,t_end;
	char * filename=args;
	// printf("%s\n",filename );
	// FILE *op = fopen(filename,"w");
	t_start = get_wall_time();
	// printf("t_start:%lf\n",t_start);
	while (1){
		
		clock_t start, end;
		double cpu_time_used;

		start = clock();
		// printf("sleep\n");
		usleep(1000000);
		// printf("awake\n");
		end = clock();
		cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
		t_end = get_wall_time();
		FILE *op = fopen(filename,"a");
		t_end = get_wall_time();
		fprintf(op,"%f, %f \n",t_end,cpu_time_used );
		fclose(op);
	}
}


//consume CPU time for a window
void * run_cpu_intensive_load(void * args){
	task *task_instance=args;
	int sno=task_instance->sno;
	float cpu_time=task_instance->cpu_time;
	float start_time=task_instance->start_time;
	float end_time=task_instance->end_time;
	char * taskID=task_instance->taskID;
	int num_splits=task_instance->num_splits;
	start_time=start_time;
	end_time=end_time;
	float runtime=cpu_time;
    float deadline=(float)(end_time-start_time);
    
    float period=deadline;
    int priority=task_instance->priority;


    int ret=-1;
    
    //classify only critical tasks using cgroups
    if(priority==9){
    	char cpu_grp[100];
		sprintf(cpu_grp,"sudo cgclassify -g cpu:group_p %d\n",getpid());
		// printf("%s\n",cpu_grp);
		system(cpu_grp);

    }
   
	int i;
	// clock_t start, end;
	struct timespec start, finish;
	double t_start,t_end;
	double total_time_spent;

	double start_wall,end_wall;
	double time_spent;

	char a[50];

	//SLEEP TIME CALCULATION 
	float ts;
	int buff=10;
	float num;
	int j=0,k=0;
	ts = ts*1000000;
	double cpu_t = runtime;

	double cpuTime = cpu_t;
	// int num_slices=cpuTime/slice;
	double slice_cput = cpuTime/((double)deadline/slice);

	float total=0;
	unsigned int total_sleep=0;

	//record start time of task
	t_start = get_wall_time();

	double total_sleep_time=(deadline-runtime)*1000000;
	

	
	unsigned int slice_sleep = total_sleep_time/ ((double)deadline/slice);
	
	// slice_sleep *= 1000000;// convert to microseconds;

	while(1)
	{
		start_wall = get_wall_time();

		//get per thread cputime until now
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

		//CPU Load
		for(j=0;j<1000000;)
		{
			num = 12/1822*4386384348/579849;
			num = 12/1822*4386384348/579849;
			num = 12/1822*4386384348/579849;
		
			//get per thread cputime until now
			clock_gettime(CLOCK_THREAD_CPUTIME_ID, &finish); 

			//find amount of cpu time consumed in the last window by thread
			long seconds = finish.tv_sec - start.tv_sec; 
	    	long ns = finish.tv_nsec - start.tv_nsec; 
	    	if (start.tv_nsec > finish.tv_nsec) { // clock underflow 
				--seconds; 
				ns += 1000000000; 
			} 

			num= (double)seconds + (double)ns/(double)1000000000;
			
			if(num>=slice_cput)
				break;
		}

		total+=num;
		if(usleep(slice_sleep)==-1){
			printf("sleep not successful\n");
		}
		total_sleep+=slice_sleep;
		if(total>=cpuTime)
			break;
		end_wall = get_wall_time();
		time_spent = end_wall - start_wall;
	}
	if(total_sleep<total_sleep_time){
		double difference=total_sleep_time-total_sleep;
		if(usleep(difference)==-1){
			printf("sleep not successful\n");
		}
		total_sleep+=difference;
	}
	t_end = get_wall_time();

	// find total time taken by task
	total_time_spent = t_end - t_start;
	printf("\nEnding: S.no: %d, taskID: %s, Input Period: %f,Delay : %f, priority: %d\n",sno,taskID,period,total_time_spent-period,priority);
	
	// Write to wl_resp the details of task
	FILE *op = fopen("wl_resp.txt","a");
	
	fprintf(op,"%d,%s,%d,%f,%f,%lf,%lf,%f,%f,%f,%f,%f,%f\n",sno,taskID,priority,start_time,end_time,t_start,t_end,runtime,period,total,total_time_spent,total_sleep_time,total_sleep/1000000.0);
	fclose(op);

	return 0;

}
 
int main(int argc, char *argv[])
{	
	
	FILE *f;
	size_t len = 0;
	char * taskID=argv[0];
	char  filename[100];
	char log_file[100];
	float start_time=0.0;
	float end_time=0.0;
	float cpu_time=0.0;
	int priority=0;
	int scheduling_class=0;
	int num_splits=1;
	int flag=0;
	float prev=0;
	int sno=0;

	int curr;
	
	int intr;

	strcpy(filename,"tasks_scaled/");
	taskID[strlen(taskID)-1]='\0';

	int count=0;

	char * line = NULL;
    
    ssize_t read;

    task *task_instance=malloc(sizeof *task_instance);
    strcpy(log_file,"cgroups_only/logs/");
    
	strcat(taskID,".txt");
	strcat(log_file,taskID);
	strcat(filename,taskID);
	f = fopen(filename,"r");


	//create thread to monitor CPU usage
	pthread_t logger_thread;
	if(pthread_create(&logger_thread,NULL,log_cpu_usage,log_file)){
        		printf("successfully created logging thread\n");
        	}

	while ((read = getline(&line, &len, f)) != -1) {

		if(!flag){
			intr = 0;
			prev = curr;
		}
		count++;

		//parse CSV File here
		char *pt;
		
	    pt = strtok (line,",");
        start_time = atof(pt)/test_scale;
        end_time= atof( strtok (NULL, ","))/test_scale;


        if(end_time>6600){
        	printf("Ending %s\n",taskID);
        	break;
        }
        if(!flag){
			intr = 0;
			prev = end_time;
			flag=1;
		}
		else{
			intr = start_time - prev;
			sleep(intr);
			prev=end_time;
		}

        
        pt=strtok (NULL, ",");
        
        pt=strtok (NULL, ",");

        //read from file
        
        priority=atoi(strtok (NULL, ","));
        
		scheduling_class=atoi(strtok (NULL, ","));
        
        cpu_time=atof(strtok(NULL,","))/test_scale;
        
        num_splits=atoi(strtok(NULL,","));
        
        cpu_time=num_splits*cpu_time/scale;

        //if cpu usage rate is greater than 1 core-second per second, split into multiple threads
        if(cpu_time>end_time-start_time){

        	double execution_time=end_time-start_time;
        	num_splits=(int)ceil(cpu_time/execution_time);
        	cpu_time=cpu_time/num_splits;
        	// printf("Util > 1. splitting %d %f\n",num_splits,cpu_time);
        }
        
        sno=atoi(strtok(NULL,","));
        pthread_t task_instances[num_splits];
        task_instance->sno=sno;
        task_instance->cpu_time=cpu_time;
        task_instance->start_time=start_time;
        task_instance->end_time=end_time;
        task_instance->taskID=strdup(taskID);
        task_instance->priority=priority;



        for (int i =0;i<num_splits;i++){
        	
        	if(pthread_create(&task_instances[i],NULL,run_cpu_intensive_load,task_instance)){
        		printf("successful\n");
        	}
        }

        //task is split into multiple threads if utilisation is more than 1. Each thread has equal utilisation
        for (int i =0;i<num_splits;i++){


        	pthread_join(task_instances[i],NULL);

        }
        printf("task completed %d\n",sno);
    }

}	


