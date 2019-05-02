// Author: Idan Twito
// ID:     311125249

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "threadPool.h"
#include "osqueue.h"

#define FILE_DESC 2
#define MALLOC_ERR_MSG "Error! memory not allocated.\n"
#define THPOOL_ERR_MSG "Error! Thread Pool is Empty !.\n"
#define ERROR_MSG "Error in system call\n"

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

static void *MissionsAllocation(void *arg) {
    ThreadPool *threadPool = (ThreadPool *) (arg);
    if (threadPool == NULL) {
        //TODO: make sure its true
        errorPrint("blab lba??");
        exit(0);
    }
    // if there are threads that wait for missions - make sure these threads don't take the
    // same mission via mutexes
    while ((!osIsQueueEmpty(threadPool->missionsQueue) && threadPool->threadPoolCondition != 0) ||
           threadPool->threadPoolCondition == 2) {
        pthread_mutex_lock(&threadPool->pthreadMutex);
        // TODO - check if while is necessary instead of if
        while (osIsQueueEmpty(threadPool->missionsQueue) && threadPool->threadPoolCondition == 2) {
            // wait until a mission is inserted to the missions queue
            pthread_cond_wait(&(threadPool->cond), &(threadPool->pthreadMutex));
        }
        // mission is assigned to thread, program can unlock mutex
        Mission *mission = osDequeue(threadPool->missionsQueue);
        pthread_mutex_unlock(&threadPool->pthreadMutex);
        mission->funcPointer(mission->arg);
        free(mission);
    }
    //TODO - should be null ?
    pthread_exit(0);
}

ThreadPool *tpCreate(int numOfThreads) {
    int i = 0;
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
    // Available for allocation mission to threads
    threadPool->threadPoolCondition = 2;
    if(pthread_cond_init(&(threadPool->cond),NULL) != 0){
        errorPrint(ERROR_MSG);
        exit(0);
    }
    if(pthread_mutex_init(&(threadPool->pthreadMutex),NULL) != 0){
        errorPrint(ERROR_MSG);
        exit(0);
    }
    for (i = 0; i < numOfThreads; i++) {
        int createResult = pthread_create(&(threadPool->pthreadArr[i]), NULL, MissionsAllocation,
                                          threadPool);
        if (createResult != 0) {
            //TODO
            errorPrint(ERROR_MSG);
            exit(0);
        }
    }
    return threadPool;
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {}

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    if (threadPool == NULL) {
        errorPrint(THPOOL_ERR_MSG);
        return 1;
    }
    if ((threadPool->threadPoolCondition != 2)) { return 0; }
    Mission *mission = (Mission *) malloc(sizeof(Mission));
    if (mission == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        exit(0);
    }
    mission->funcPointer = computeFunc;
    mission->arg = param;
    pthread_mutex_lock(&(threadPool->pthreadMutex));
    int flag = 0;
    if (osIsQueueEmpty(threadPool->missionsQueue)) { flag = 1; }
    osEnqueue(threadPool->missionsQueue, mission);
    // notify all threads that the mission queue is no longer empty
    if (flag == 1) {
        pthread_cond_broadcast(&(threadPool->cond));
        flag = 0;
    }
    pthread_mutex_unlock(&(threadPool->pthreadMutex));
    return 0;
}