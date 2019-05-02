// Author: Idan Twito
// ID:     311125249

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "threadPool.h"
#include "osqueue.h"

#define FILE_DESC 2
#define MALLOC_ERR_MSG "Error! memory not allocated."
#define THPOOL_ERR_MSG "Error! Thread Pool is Empty !."

/**
 * Function Name: errorPrint
 * Function Input: char *errorMsg
 * Function Output: void
 * Function Operation: writes the error message which errorMsg contains
 */
void errorPrint(char *errorMsg) {
    size_t size = strlen(errorMsg);
    write(FILE_DESC, errorMsg, size);
}

static void missionsAllocation(void* arg){
    ThreadPool *threadPool = (ThreadPool*)(arg);
    if(threadPool == NULL){
        //TODO: make sure its true
        errorPrint("blab lba??");
        exit(0);
    }
}

ThreadPool *tpCreate(int numOfThreads) {
    if (numOfThreads < 1) { return NULL; }
    ThreadPool *threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        //TODO: make sure its correct
        exit(0);
    }
    threadPool->threadsAmount = numOfThreads;
    threadPool->pthreadArr = (pthread_t *) malloc(numOfThreads * sizeof(pthread_t));
    if (threadPool->pthreadArr == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        free(threadPool);
        //TODO: make sure its correct
        exit(0);
    }
    threadPool->missionsQueue = osCreateQueue();
    return threadPool;
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {}

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    if(threadPool == NULL){
        errorPrint(THPOOL_ERR_MSG);
        return 1;
    }
    Mission *mission = (Mission *) malloc(sizeof(Mission));
    if (mission == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        exit(0);
    }
    mission->funcPointer = computeFunc;
    mission->arg = param;
    osEnqueue(threadPool->missionsQueue, mission);
}