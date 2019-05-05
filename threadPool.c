// Author: Idan Twito
// ID:     311125249

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include "threadPool.h"
#include "osqueue.h"

#define FILE_DESC 2
#define MALLOC_ERR_MSG "Error! memory not allocated.\n"
#define THPOOL_ERR_MSG "Error! Thread Pool is Empty !\n"
#define THPOOL_NULL_MSG "Error! Thread Pool is Null !\n"
#define ERROR_MSG "Error in system call\n"
#define THREADPOOL_FORCE_EXIT 0
#define THREADPOOL_RUNNING 1
#define THREADPOOL_WAIT_FOR_QUEUE 2

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
        errorPrint(THPOOL_NULL_MSG);
        exit(1);
    }
    // if there are threads that wait for missions - make sure these threads don't take the
    // same mission via mutexes
    while ((!(osIsQueueEmpty(threadPool->missionsQueue)) && threadPool->threadPoolCondition !=
                                                            THREADPOOL_FORCE_EXIT) ||
           (threadPool->threadPoolCondition == THREADPOOL_RUNNING)) {
        pthread_mutex_lock(&(threadPool->pthreadMutex));
        // TODO - check if while is necessary instead of if
        while (osIsQueueEmpty(threadPool->missionsQueue) && threadPool->threadPoolCondition ==
        THREADPOOL_RUNNING) {
            // wait until a mission is inserted to the missions queue
            pthread_cond_wait(&(threadPool->cond), &(threadPool->pthreadMutex));
        }
        // mission is assigned to thread, then the program unlocks mutex
        Mission *mission = osDequeue(threadPool->missionsQueue);
        pthread_mutex_unlock(&threadPool->pthreadMutex);
        if (mission != NULL) {
            mission->funcPointer(mission->arg);
            free(mission);
        }
    }
    pthread_exit(0);
}

ThreadPool *tpCreate(int numOfThreads) {
    int i = 0;
    if (numOfThreads < 1) { return NULL; }
    ThreadPool *threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        //TODO: make sure its correct
        _exit(0);
    }
    threadPool->threadsAmount = numOfThreads;
    threadPool->pthreadArr = (pthread_t *) malloc(numOfThreads * sizeof(pthread_t));
    if (threadPool->pthreadArr == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        free(threadPool);
        //TODO: make sure its correct
        _exit(0);
    }
    threadPool->missionsQueue = osCreateQueue();
    // Allocating mission to threads is available
    threadPool->threadPoolCondition = THREADPOOL_RUNNING;
    if (pthread_cond_init(&(threadPool->cond), NULL) != 0) {
        //TODO
        errorPrint(ERROR_MSG);
        _exit(0);
    }
    if (pthread_mutex_init(&(threadPool->pthreadMutex), NULL) != 0) {
        //TODO: make sure its correct

        errorPrint(ERROR_MSG);
        _exit(0);
    }
    for (i = 0; i < numOfThreads; i++) {
        int createResult = pthread_create(&(threadPool->pthreadArr[i]), NULL, MissionsAllocation,
                                          threadPool);
        if (createResult != 0) {
            //TODO: make sure its correct
            errorPrint(ERROR_MSG);
            _exit(0);
        }
    }
    return threadPool;
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    if (threadPool == NULL) {
        errorPrint(THPOOL_ERR_MSG);
        return;
    }
    if (threadPool->threadPoolCondition != THREADPOOL_RUNNING) { return; }
    int i;
    pthread_mutex_lock(&(threadPool->pthreadMutex));
    if (shouldWaitForTasks != 0) {
        threadPool->threadPoolCondition = THREADPOOL_WAIT_FOR_QUEUE;
    } else {
        threadPool->threadPoolCondition = THREADPOOL_FORCE_EXIT;
        while (!(osIsQueueEmpty(threadPool->missionsQueue))) {
            Mission *mission = osDequeue(threadPool->missionsQueue);
            free(mission);
        }
    }
    pthread_cond_broadcast(&(threadPool->cond));
    pthread_mutex_unlock(&(threadPool->pthreadMutex));
    for (i = 0; i < threadPool->threadsAmount; i++) {
        pthread_join(threadPool->pthreadArr[i], NULL);
    }

    osDestroyQueue(threadPool->missionsQueue);
    free(threadPool->pthreadArr);
    pthread_cond_destroy(&threadPool->cond);
    pthread_mutex_destroy(&threadPool->pthreadMutex);
    free(threadPool);
}

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    if (threadPool == NULL) {
        errorPrint(THPOOL_ERR_MSG);
        //TODO
        return -1;
    }
    if ((threadPool->threadPoolCondition != THREADPOOL_RUNNING)) { return -1; }
    Mission *mission = (Mission *) malloc(sizeof(Mission));
    if (mission == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        //TODO
        return -1;
    }
    mission->funcPointer = computeFunc;
    mission->arg = param;
    pthread_mutex_lock(&(threadPool->pthreadMutex));
    int flag = 0;
    if (osIsQueueEmpty(threadPool->missionsQueue)) { flag = 1; }
    osEnqueue(threadPool->missionsQueue, mission);
    // notify all threads that the mission queue is no longer empty
    // Todo: make sure it's working !
    if (flag == 1) {
        flag = 0;
        //TODO PRINT..
        if (pthread_cond_broadcast(&(threadPool->cond)) != 0) { return -1; }
    }
    pthread_mutex_unlock(&(threadPool->pthreadMutex));
    return 0;
}