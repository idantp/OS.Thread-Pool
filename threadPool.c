// Author: Idan Twito
// ID:     311125249

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "threadPool.h"

#define FILE_DESC 2
#define MALLOC_ERR_MSG "Error! memory not allocated."

/**
 * Function Name: errorPrint
 * Function Input: void
 * Function Output: void
 * Function Operation: writes a system call error message
 */
void errorPrint(char *errorMsg) {
    size_t size = strlen(errorMsg);
    write(FILE_DESC, errorMsg, size);
}

ThreadPool *tpCreate(int numOfThreads) {
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
        //TODO: make sure its correct
        exit(0);
    }
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {}

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {}