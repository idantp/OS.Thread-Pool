// Author: Idan Twito
// ID:     311125249

#ifndef __THREAD_POOL__
#define __THREAD_POOL__
#include "osqueue.h"
#include <pthread.h>

typedef struct mission {
    void *arg;

    void (*funcPointer)(void *);
} Mission;

typedef struct thread_pool {
    int threadsAmount;
    OSQueue *missionsQueue;
    pthread_t *pthreadArr;
    pthread_mutex_t pthreadMutex;
    // 0 = indicates that the threads cannot be assigned with new mission in the queue
    // 1 = indicates that the thread pool is running
    // 2 = indicates that the thread pool will finish the mission in the queue but is not
    //     available for new missions in the queue
    int threadPoolCondition;
    pthread_cond_t cond;
} ThreadPool;

ThreadPool *tpCreate(int numOfThreads);

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param);

#endif
