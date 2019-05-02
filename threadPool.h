// Author: Idan Twito
// ID:     311125249

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.c"
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
} ThreadPool;

ThreadPool *tpCreate(int numOfThreads);

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param);

#endif
