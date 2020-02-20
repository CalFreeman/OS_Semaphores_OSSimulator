#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<pthread.h>
#include<semaphore.h>


        //ctrl-c sig handler
void sigHandler();
void signalHandler();
int exitFlag = 0;
void sigAlarmHandler(int sig_num);
int temp = 0;   // number of child processes
pid_t childPid;

        //simulated system clock structure
typedef struct system_clock {
        int seconds;
        int nanoSeconds;
        int shmMsg;
        int shmMsgSeconds;
        sem_t mutex;
} system_clock;
        //ptr declration for using struct in shared memory
static system_clock* shmClockPtr;

        //message structure
typedef struct system_msg {
        int type;
        char string[256];
} system_msg;
static system_msg* shmMsgPtr;


        //globals for getopt
static int childX = 0;
static int timeZ = 0;


int main(int argc, char *argv[]){

                //log file
        FILE *ofp = NULL;
        char *ofname = NULL;
                //default file name
        ofname = "output.dat";

        //sig handler for exiting
        signal(SIGINT, signalHandler);

        // child pid variables
        pid_t child_pid, wpid;
        int childPidsArr[1024];
        int running_processes = 0;
        int processCount = 0;
        int status = 0;
        // command line variables
        int opt;
        int logFileFlag = 0;
        const char *output;
        int sflag = 0;
        int tflag = 0;


        while((opt = getopt(argc, argv, "hs:l:t:")) != -1){
                switch(opt){
                        case 'h':
                                printf("Usage: ./oss [-h] [-s x] [-l [filename]] [-t z]\n");
                                printf("-h,   --help                    Print a help message and exit.\n");
                                printf("-s x, --child processes         where x is the maximum number of child processes spawned.\n");
                                printf("-l,   --log file                -l [filename] log file used.\n");
                                printf("-t z, --termination             -t z, parameter z is the time in real seconds when the master will\n");
                                printf("                                terminate itself and all children.\n");
                                abort();
                        case 's':
                                sflag = 1;
                                childX = atoi(optarg);
                                break;
                        case 'l':
                                logFileFlag = 0;
                                ofname = optarg;
                                break;
                        case 't':
                                tflag = 1;
                                timeZ = atoi(optarg);
                                break;
                        case ':':
                                printf("option needs a value\n");
                                break;
                        case '?':
                                printf("unknown option: %c\n", optopt);
                        break;
                }
        }
        printf("optind and argc: %d %d \n", argc, optind);
        printf("-s z & -t z: %d %d \n", childX, timeZ);
        printf("filename: %s \n", output);
        //argc statement determines if a file was supplied
        if(argc > optind){
                //a file was suppled
        } else if(optind != 1){
                //
        } else {

        }

                // OPENING OUTPUT FILE
        ofp = fopen(ofname, "w");
        if(ofp == NULL){
                perror(argv[0]);
                perror("output file: Error: output file failed to open.");
                return EXIT_FAILURE;
        }


        //default enviroment
        if(sflag == 0){
                childX = 5;
        }
        if(tflag == 0){
                timeZ = 5;
        }
        // END OF COMMANDLINE

        // SHARED MEMORY
        int shmid;
        key_t key;
        key = 5678;
        void *tmp_ptr = (void *)0; // equal to null pointer constant
        //generate key
        key = ftok(".", 'a');

        //create the segment
        shmid = shmget(key, sizeof(system_clock), IPC_CREAT | 0666);
        if(shmid < 0){
                perror("Error: shmget");
                exit(EXIT_FAILURE);
        }
        // now we attach the segment to our data space
        shmClockPtr = (system_clock*) shmat(shmid, NULL, 0);
        if(shmClockPtr == (void *)-1){
                perror("Error: shmat");
                exit(EXIT_FAILURE);
        } else {
                shmClockPtr->seconds = 0;
                shmClockPtr->nanoSeconds = 0;
        }

        // program termination based on real time, default 5.
        int maxTime = timeZ; // setting max time for master processes
        alarm(maxTime);
        signal(SIGALRM, sigAlarmHandler);

        // Now initiate semaphore
        sem_init(&(shmClockPtr->mutex), 1, 1);
        shmClockPtr->shmMsgSeconds = 0;
        shmClockPtr->shmMsg = 0;

        // FORKING CHILDREN
        // just removed shmClockPtr->seconds < 2)
        //while(exitFlag != 1 && running_processes < 100){      // shmClockPtr->seconds is breaking the entire thing if too small
        while(processCount <= 100){
                shmClockPtr->nanoSeconds += 20000; // increment 20000 nanoseconds per loop
                if(shmClockPtr->nanoSeconds >= 1000000000){
                        shmClockPtr->seconds++;
                        shmClockPtr->nanoSeconds = 0;
                }
                //printf("incrementing %d\n", shmClockPtr->seconds);
                //printf("seconds: %ld\n", shmClockPtr->seconds);
                while(running_processes < (childX-1)){ // if number of running_processes is less than max specified
                        child_pid = fork();
                        //shmClockPtr->shmMsg = 0;
                        running_processes++; //count of running processes
                        if(child_pid == -1){
                                perror("ForkError: Error: failed to fork child_pid = -1");
                                return (EXIT_FAILURE);
                        } else if(child_pid == 0){
                                //CHILD PROCESSES
                                //need to call ./user.c and execl
                                temp++;
                                execl("./user", NULL);
                                perror("Execl Error: Error: failed to exec child from the fork of oss");
                                exit(EXIT_FAILURE); //child prog failed to exec
                        } else {
                                // PARENT PROCESSES
                                childPidsArr[processCount] = child_pid;
                                childPid = child_pid;   // for master time signal termination
                                processCount++;
                        }
                        //processCount++;
                } //if (running_processes < chilX
                if(running_processes > childX){
                        printf("max kids atm\n");
                }
                        // reading shared memory
                if(shmClockPtr->shmMsg != 0){
                        fprintf(ofp, "Master: Child pid is terminating at my time ");
                        fprintf(ofp, "%d.%d because it reached %d.%d in child process\n", shmClockPtr->seconds, shmClockPtr->nanoSeconds, shmClockPtr->shmMsgSeconds, shmClockPtr->shmMsg);
                        //printf("smgMsgSeconds: %d ,", shmClockPtr->shmMsgSeconds);
                        //printf("smgMsg: %d\n", shmClockPtr->shmMsg);
                        shmClockPtr->shmMsg = 0;
                        shmClockPtr->shmMsgSeconds = 0;
                }

                //if (( wpid = waitpid(-1, &status, WNOHANG)) > 0){
                //        running_processes--;
                //}

                // break if signal
                if(exitFlag == 1){
                        break;
                }

                // break if time exceeds 2 seconds in system
                if( shmClockPtr->seconds >= 2){
                        //sigHandler();
                        break;
                }
                //printf("processess count %d\n", processCount);
                if( processCount >= 100 ){
                        break;
                }

                if (( wpid = waitpid(-1, &status, WNOHANG)) > 0){
                        running_processes--;
                }


        } // main while loop
        while((wpid = wait(&status)) > 0);


        // Free allocated memory
        shmdt(shmClockPtr);
        shmctl(shmid, IPC_RMID, NULL);
        sigHandler();
        printf("oss.c done\n");

        return 0;
}

// ctrl-c handler
void signalHandler() {
        exitFlag = 1;
        printf("signal handler executing.\n");
        int i;
        for(i = 0; i < temp; i++){
                kill(childPid, SIGKILL);
        }
        signal(SIGINT, signalHandler);
}
// main loop alarm() handler
void sigAlarmHandler(int sig_num){
  printf("signal alarm exit.\n");
  exitFlag = 1;
  int i;
  printf("-t time exceeded\n");
  for(i = 0; i < temp; i++){
    kill(childPid, SIGKILL);
  }
  //shmdt(shmClockPtr);
  //shmctl(shmid, IPC_RMID, NULL);
  signal(SIGALRM, sigAlarmHandler);

}


void sigHandler(){
        exitFlag = 1;
        printf("Exit sigHandler executing.\n");
        int i;
        for(i = 0; i < temp; i++){
                kill(childPid, SIGKILL);
        }
        //shmdt(shmClockPtr);
        //shmctl(shmid, IPC_RMID, NULL);
        //exit(EXIT_SUCCESS);
}