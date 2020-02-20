#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
typedef struct system_clock {
        int seconds;
        int nanoSeconds;
        int shmMsg;
        int shmMsgSeconds;
        sem_t mutex;
} system_clock;
//ptr declration for using struct in shared memory
static system_clock* shmClockPtr;
void signalHandler();
int getOutFlag = 0;

int main(int argc, char *argv[]){
        //printf("inside child_pid\n");
        signal(SIGINT, signalHandler);
        // SHARED MEMORY
        int shmid;
        key_t key;
        key = ftok(".", 'a');
        shmid = shmget(key, sizeof(system_clock), 0600 | IPC_CREAT);
        if(shmid < 0){
                perror("Error: shmget child");
                return(EXIT_FAILURE);
        }
        shmClockPtr = (system_clock*) shmat(shmid, NULL, 0);
        if(shmClockPtr == (void *)-1){
                perror("Error: shmatchild");
                exit(EXIT_FAILURE);
        }

        // reading the simulated time system clock generated by oss.
        int tempSeconds = shmClockPtr->seconds;
        int tempNanos = shmClockPtr->nanoSeconds;
        srand(time(NULL)); //seed random
        int r = rand() % 1000000; // random termination time
        tempNanos = tempNanos + r; // adding random termination time
        if( tempNanos >= 1000000000){ //checking for time overflow
                tempNanos = tempNanos - 1000000000;
                tempSeconds = tempSeconds + 1;
        }
        //printf("goig into childwhile\n");
        int childStuck = 0;
        while(getOutFlag < 1){
                        // CRITICAL SECTION
                sem_wait(&(shmClockPtr->mutex));
                //printf("have %d %d\n", shmClockPtr->seconds, shmClockPtr->nanoSeconds);
                //printf("looking for:%d %d\n", tempSeconds, tempNanos);
                long double totalTime = shmClockPtr->seconds + (shmClockPtr->nanoSeconds / 1000000000);
                long double totalTimeTemp = tempSeconds + (tempNanos / 1000000000);
                //printf("trying totaltime >= totaltimetemp %d %d\n", totalTime, totalTimeTemp);
                while(totalTime >= totalTimeTemp && shmClockPtr->seconds >= tempSeconds){
                        //printf("clock time based exit: %ld \n", shmClockPtr->nanoSeconds);
                        if(shmClockPtr->shmMsg == 0 && shmClockPtr->shmMsgSeconds == 0){
                                getOutFlag = 1;
                                shmClockPtr->shmMsg = shmClockPtr->nanoSeconds;
                                shmClockPtr->shmMsgSeconds = shmClockPtr->seconds;
                                printf("now: %d %d\n", shmClockPtr->seconds, shmClockPtr->nanoSeconds);
                                //sem_post(&(shmClockPtr->mutex));
                                break;
                        }
                }
                sem_post(&(shmClockPtr->mutex));
                childStuck++;
                //printf("waiting . . .\n");
        }

        //Deatch shared memory
        shmdt(shmClockPtr);

        return EXIT_SUCCESS;
}

void signalHandler() {
        getOutFlag = 1;
        printf("signal handler executing.\n");
        signal(SIGINT, signalHandler);
}