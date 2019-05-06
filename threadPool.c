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
#define MALLOC_ERR_MSG "Error in memory allocation.\n"
#define THPOOL_ERR_MSG "Error! incorrect number of threads !\n"
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

/**
 * Function Name: MissionsAllocation
 * Function Input: void *arg - indicates a Thread Pool
 * Function Output: None
 * Function Operation:
 */
static void *MissionsAllocation(void *arg) {
    ThreadPool *threadPool = (ThreadPool *) (arg);
    if (threadPool == NULL) {
        errorPrint(THPOOL_NULL_MSG);
        exit(1);
    }
    // while threads wait for new missions in the queue - make sure these threads don't take the
    // same mission via mutexes and assign each available thread a mission if the missions queue
    // is not empty.
    while ((!(osIsQueueEmpty(threadPool->missionsQueue)) && threadPool->threadPoolCondition !=
                                                            THREADPOOL_FORCE_EXIT) ||
           (threadPool->threadPoolCondition == THREADPOOL_RUNNING)) {
        if ((pthread_mutex_lock(&(threadPool->pthreadMutex))) != 0) {
            errorPrint(ERROR_MSG);
            exit(1);
        }
        // as long as the missions queue is empty and the thread pool is running - wait for
        // new mission
        while (osIsQueueEmpty(threadPool->missionsQueue) && threadPool->threadPoolCondition ==
                                                            THREADPOOL_RUNNING) {
            // wait until a mission is inserted to the missions queue
            pthread_cond_wait(&(threadPool->cond), &(threadPool->pthreadMutex));
        }
        // mission is assigned to thread, then the program unlocks mutex
        Mission *mission = osDequeue(threadPool->missionsQueue);
        if ((pthread_mutex_unlock(&(threadPool->pthreadMutex))) != 0) {
            errorPrint(ERROR_MSG);
            exit(1);
        }
        //
        if (mission != NULL) {
            // implement the given mission
            mission->funcPointer(mission->arg);
            free(mission);
        }
    }
    pthread_exit(0);
}

/**
 * Function Name: tpCreate
 * Function Input: int numOfThreads - indicates the number of threads in the Thread Pool
 * Function Output: ThreadPool * - A pointer to the Thread Pool that was created
 * Function Operation: the function creates a Thread Pool which is consists of the threads
 *                     amount that was given as a parameter. Then it initializes each thread
 *                     via MissionsAllocation Function.
 */
ThreadPool *tpCreate(int numOfThreads) {
    int i = 0;
    // if the threads amount is less than 1 - exit
    if (numOfThreads < 1) {
        errorPrint(THPOOL_ERR_MSG);
        exit(1);
    }
    // if an error occurred in memory allocation - exit
    ThreadPool *threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        exit(1);
    }
    threadPool->threadsAmount = numOfThreads;
    // if an error occurred in memory allocation - exit
    threadPool->pthreadArr = (pthread_t *) malloc(numOfThreads * sizeof(pthread_t));
    if (threadPool->pthreadArr == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        free(threadPool);
        exit(1);
    }
    // create the mission Queue for the Thread Pool
    threadPool->missionsQueue = osCreateQueue();
    // Making allocation missions to threads is available
    threadPool->threadPoolCondition = THREADPOOL_RUNNING;
    if (pthread_cond_init(&(threadPool->cond), NULL) != 0) {
        free(threadPool->pthreadArr);
        osDestroyQueue(threadPool->missionsQueue);
        free(threadPool);
        errorPrint(ERROR_MSG);
        exit(1);
    }
    if (pthread_mutex_init(&(threadPool->pthreadMutex), NULL) != 0) {
        free(threadPool->pthreadArr);
        osDestroyQueue(threadPool->missionsQueue);
        pthread_cond_destroy(&threadPool->cond);
        free(threadPool);
        errorPrint(ERROR_MSG);
        exit(1);
    }
    // allocating each thread a mission
    for (i = 0; i < numOfThreads; i++) {
        int createResult = pthread_create(&(threadPool->pthreadArr[i]), NULL, MissionsAllocation,
                                          threadPool);
        // if something went wrong while trying to create a pthread - exit.
        if (createResult != 0) {
            free(threadPool->pthreadArr);
            osDestroyQueue(threadPool->missionsQueue);
            pthread_cond_destroy(&threadPool->cond);
            pthread_mutex_destroy(&threadPool->pthreadMutex);
            free(threadPool);
            errorPrint(ERROR_MSG);
            exit(1);
        }
    }
    // return the Thread Pool.
    return threadPool;
}

/**
 * Function Name: tpDestroy
 * Function Input:
 * Function Output: None
 * Function Operation:
 */
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    if (threadPool == NULL) {
        errorPrint(THPOOL_ERR_MSG);
        exit(1);
    }
    if (threadPool->threadPoolCondition != THREADPOOL_RUNNING) { return; }
    int i;
    if ((pthread_mutex_lock(&(threadPool->pthreadMutex))) != 0) {
        errorPrint(ERROR_MSG);
        exit(1);
    }
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
    if ((pthread_mutex_unlock(&(threadPool->pthreadMutex))) != 0) {
        errorPrint(ERROR_MSG);
        exit(1);
    }
    for (i = 0; i < threadPool->threadsAmount; i++) {
        pthread_join(threadPool->pthreadArr[i], NULL);
    }
    osDestroyQueue(threadPool->missionsQueue);
    free(threadPool->pthreadArr);
    pthread_cond_destroy(&threadPool->cond);
    pthread_mutex_destroy(&threadPool->pthreadMutex);
    free(threadPool);
}

/**
 * Function Name: tpInsertTask
 * Function Input: Thread Pool, *computeFunc - mission(function), the mission's parameter
 * Function Output: -1 if the Thread Pool was already instructed to be destroyed
 *                   0 if the mission insertion to the queue succeeded
 * Function Operation:
 */
int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    if (threadPool == NULL) {
        errorPrint(THPOOL_NULL_MSG);
        exit(1);
    }
    // making sure that inserting new missions to the queue's missions is impossible
    // once the Thread Pool is destroyed
    if ((threadPool->threadPoolCondition != THREADPOOL_RUNNING)) { return -1; }
    // creating a new mission based on the parameters of this function
    Mission *mission = (Mission *) malloc(sizeof(Mission));
    if (mission == NULL) {
        errorPrint(MALLOC_ERR_MSG);
        exit(1);
    }
    mission->funcPointer = computeFunc;
    mission->arg = param;
    if ((pthread_mutex_lock(&(threadPool->pthreadMutex))) != 0) {
        errorPrint(ERROR_MSG);
        exit(1);
    }
    int flag = 0;
    // insert the new mission to the queue
    if (osIsQueueEmpty(threadPool->missionsQueue)) { flag = 1; }
    osEnqueue(threadPool->missionsQueue, mission);
    // notify all threads that the missions queue is no longer empty (in Mission
    // Allocation function)
    if (flag == 1) {
        flag = 0;
        if (pthread_cond_broadcast(&(threadPool->cond)) != 0) {
            errorPrint(ERROR_MSG);
            exit(1);
        }
    }
    if ((pthread_mutex_unlock(&(threadPool->pthreadMutex))) != 0) {
        errorPrint(ERROR_MSG);
        exit(1);
    }
    return 0;
}