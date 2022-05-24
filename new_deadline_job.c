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

int schedule_job(float runtime, float period, float deadline){
	
		struct sched_attr attr;
		unsigned int flags = 0;
		// unsigned int runtime_u=runtime*1000*1000*1000;
		// printf("In Function() %d",(int)(runtime*1000*1000));

		attr.size = sizeof(attr);
		attr.sched_flags = 0;
		attr.sched_nice = 0;
		attr.sched_priority = 0;

		if(runtime>deadline){
			runtime=deadline;
		}
		attr.sched_policy = SCHED_DEADLINE;
		attr.sched_runtime=(__u64)((runtime*1000.0*1000.0)*1000);
		attr.sched_period = (__u64)((period*1000.0*1000.0)*1000);
		attr.sched_deadline=(__u64)((deadline*1000.0*1000.0)*1000);
		
		// printf("In Function %u, %u, %u\n", attr.sched_runtime,attr.sched_period,attr.sched_deadline);
		return sched_setattr(0, &attr, flags);

}

void * log_cpu_usage(void * args){
	double t_start,t_end;
	char * filename=args;
	t_start = get_wall_time();
	while (1){
		
		clock_t start, end;
		double cpu_time_used;

		start = clock();
		\
		usleep(1000000);
		
		end = clock();
		cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
		t_end = get_wall_time();
		FILE *op = fopen(filename,"a");
		t_end = get_wall_time();
		fprintf(op,"%f, %f \n",t_end,cpu_time_used );
		fclose(op);
	}
}

void * run_cpu_intensive_load(void * args){
	int i;
	
	struct timespec start, finish;
	double t_start,t_end;
	double util_start,util_end;
	double total_time_spent;

	double start_wall,end_wall;
	double time_spent;

	char a[50];

	int flag=0;// to differentiate between profiling and execution stages

	task *task_instance=args;
	int sno=task_instance->sno;
	float cpu_time=task_instance->cpu_time;
	float start_time=task_instance->start_time;
	float end_time=task_instance->end_time;
	char * taskID=task_instance->taskID;
	int num_splits=task_instance->num_splits;
	
	float runtime=cpu_time;
	float assigned_util=0.0;
    float deadline=(float)(end_time-start_time);
    
    float period=deadline;
    int priority=task_instance->priority;
    //record start time of task
	t_start = get_wall_time();

    int ret=-1;
    // schedule production jobs using EDF
    //run profiling stage by allocating one full core to task for 0.1 seconds
    if(priority==9){
		ret=schedule_job(0.01,0.01,0.01);
    	while(ret<0){
    		perror("0.1,0.1,0.1");
    		usleep(10000);
    		ret=schedule_job(0.01,0.01,0.01);
    	}	
    }

    
    
	printf("\nStarting: S.No: %d,taskID: %s, Start Time: %f, Period: %f, Runtime: %f, Ratio: %f\n",sno,taskID,start_time,period,runtime,runtime/period);
	// printf("\nStarting: taskID: %s,Start Time: %f Thread: %d\n",taskID,start_time,pthread_self());
	
	//SLEEP TIME CALCULATION 
	float ts;
	int buff=10;
	float num;
	int j=0,k=0;
	ts = ts*1000000;
	double cpu_t = runtime;

	double cpuTime = cpu_t;
	double slice_cput = cpuTime/((double)deadline/slice);

	float total=0;
	float reset_total=0;
	unsigned int total_sleep=0;
	float current_util=0.0;

	

	double total_sleep_time=(deadline-runtime)*1000000;
	// printf("total sleep time: %f\n",total_sleep_time);

	
	unsigned int slice_sleep = total_sleep_time/ ((double)deadline/slice);
	
	while(1)
	{
		srand(time(0));
		start_wall = get_wall_time();
		util_start=get_wall_time();
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

		
		if(usleep(slice_sleep)==-1){
			printf("sleep not successful\n");
		}
		total+=num;
		
		if(priority==9 && !flag)// if profiling stage 
		{
			
			float time_remaining;
			util_end = get_wall_time();
			time_spent = util_end-util_start;
			current_util=num/time_spent*1.2;
			usleep(100000);
			printf("\nS.No: %d, Required: %f, Assigned: %f\n",sno, cpuTime/deadline,current_util );
			ret=schedule_job(current_util,1,1);
	    	while(ret<0){
	    		perror("sched_setattr");
	    		usleep(10000);
	    		ret=schedule_job(current_util,1,1);

	    	}
	    	flag=1;// next up : execution stage
			
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
	if(priority==9){
		printf("\nEnding: S.no: %d, taskID: %s, Input Period: %f,Delay : %f, Runtime: %f, priority: %d\n",sno,taskID,period,total_time_spent-period,runtime,priority);
	}
	
	printf("S.no %d,Ending: taskID: %s,Start Time: %f, Thread: %d\n",sno,taskID,start_time,pthread_self());
	// Write to wl_resp the details of task
	FILE *op = fopen("wl_resp.txt","a");
	// fprintf(op,"SNo,Task Index, Priority, Start Time, End Time, Start Wall Time, End Wall Time, Runtime,Period,Actual Runtime,Actual Period,Sleep Time, Actual Sleep Time\n");
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
    strcpy(log_file,"new_deadline_only/logs/");
    
	strcat(taskID,".txt");
	strcat(log_file,taskID);
	strcat(filename,taskID);
	f = fopen(filename,"r");


	//create thread to monitor CPU usage
	pthread_t logger_thread;
	if(pthread_create(&logger_thread,NULL,log_cpu_usage,log_file)){
        		// printf("successful\n");
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
        start_time = atof(pt);
        end_time= atof( strtok (NULL, ","));


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

        // printf("end_time: %d\n", end_time);
        pt=strtok (NULL, ",");
        // printf(" job: %s\n",pt );
        pt=strtok (NULL, ",");
        // printf("task: %s\n",pt );
        priority=atoi(strtok (NULL, ","));
        // printf("priority: %d\n",priority );
		scheduling_class=atoi(strtok (NULL, ","));
        // printf("priority: %d\n",scheduling_class );
        cpu_time=atof(strtok(NULL,","));
        // printf("cpu_time: %f\n",cpu_time );
        num_splits=atoi(strtok(NULL,","));
        // printf("num_splits %d\n",num_splits );
        cpu_time=num_splits*cpu_time/scale;
        if(cpu_time>end_time-start_time){

        	double execution_time=end_time-start_time;
        	num_splits=(int)ceil(cpu_time/execution_time);
        	cpu_time=cpu_time/num_splits;
        	// printf("Util > 1. splitting %d %f\n",num_splits,cpu_time);
        }
        // printf("num_splits:%d\n",num_splits );
        sno=atoi(strtok(NULL,","));
        // num_splits=1;
        pthread_t task_instances[num_splits];
        task_instance->sno=sno;
        task_instance->cpu_time=cpu_time;
        task_instance->start_time=start_time;
        task_instance->end_time=end_time;
        task_instance->taskID=strdup(taskID);
        task_instance->priority=priority;

        // printf("%f, %f, %f\n",task_instance->cpu_time,task_instance->start_time,end_time );

        for (int i =0;i<num_splits;i++){
        	// printf("num_splits %d i %d\n",num_splits,i );
        	if(pthread_create(&task_instances[i],NULL,run_cpu_intensive_load,task_instance)){
        		// printf("successful\n");
        	}
        }

        //task is split into multiple threads if utilisation is more than 1. Each thread has half the utilisation
        for (int i =0;i<num_splits;i++){


        	pthread_join(task_instances[i],NULL);

        }
        // printf("task completed %d\n",sno);
    }

}	

