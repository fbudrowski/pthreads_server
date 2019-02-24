/**
 * @author Franciszek Budrowski
 * Main server thread, whose purpose is to match services into pairs (assignment's requirement)
 * and move them to a new 'worker' thread.
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include "report_api.h"
#include "worker.h"

ServerStorage* globalStorage;
int openPipeGlobal = -1;

void cleanUpThreadsGlobal(){
    fprintf(stderr, "Received an interrupt signal. Waiting for resources to be returned, server will terminate afterwards.\n");
    DEBUG("SIGINT %d %d\n", getpid(), (int) pthread_self());
    cleanUpThreads(globalStorage);
}

void finishMain(ServerStorage* storage, int pipe, bool doExit){
    DEBUG("Finish main thread\n");

    destroyStorage(storage);
    if (pipe != -1) close(pipe);
    unlink(MAIN_PIPE_NAME);
    if(doExit) exit(1);
}

void finishMainGlobal(){
    finishMain(globalStorage, openPipeGlobal, false);
}


void waitForAllThreads (void* data){
    DEBUG("Waiting for threads\n");
    ServerStorage* storage = globalStorage;
    CALL(pthread_mutex_lock(&storage->listProtection), exit(1));
    DEBUG("Locked\n");
    Node* nd = storage->listOfThreads.first;
    while(true){
        if (nd == NULL){
            CALL(pthread_mutex_unlock(&storage->listProtection), exit(1));
            break;
        }
        pthread_t thr = nd->thread;
        CALL(pthread_mutex_unlock(&storage->listProtection), exit(1));
        if (thr != pthread_self()){
            DEBUG("For thread %lu\n", thr);
            pthread_join(thr, NULL);
        }
        CALL(pthread_mutex_lock(&storage->listProtection), exit(1));
        nd = nd->nextNode;
    }

}

void closePipeGlobal(void* data){
    DEBUG("Closing pipe\n");
    char buffer[150];
    while(read(openPipeGlobal, buffer, 120) > 0){
        int pid, a, b;
        sscanf(buffer, "%d%d%d", &a, &b, &pid);
        DEBUG("Scanned pid %d\n", pid);
        if (pid <= 0) break;
        sendFailMessageTo(pid);
    }
    if (openPipeGlobal > 2) {
        close(openPipeGlobal);
        openPipeGlobal = -1;
    }
    DEBUG("Closed pipe\n");
}

void initialIntAction(){
    destroyStorage(globalStorage);
    unlink(MAIN_PIPE_NAME);
    exit(0);
}


int main(int argv, char** argc) {
    SYS(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
    DEBUG("ARGS %d %s %s\n", argv, argc[1], argc[2]);
    if (argv != 3) return 0;
    int k = (int) strtol(argc[1], NULL, 10);
    int n = (int) strtol(argc[2], NULL, 10);
    ServerStorage* storage = malloc(sizeof(ServerStorage));
    globalStorage = storage;
    initStorage(storage, k, n);

    SYS(mkfifo(MAIN_PIPE_NAME, 0666));
    struct sigaction saction;
    saction.sa_handler = initialIntAction;
    CALL(sigaction(SIGINT, &saction, NULL), finishMain(storage, -1, true));


    int pipeDesc = -1;


    server_ready(getpid());


    pipeDesc = open(MAIN_PIPE_NAME, O_RDONLY);
    if (pipeDesc <= 2) finishMain(storage, pipeDesc, true);
    openPipeGlobal = pipeDesc;

    SYS(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
    saction.sa_handler = cleanUpThreadsGlobal;
    CALL(sigaction(SIGINT, &saction, NULL), finishMain(storage, -1, true));
    pthread_cleanup_push(finishMainGlobal, NULL);
    pthread_cleanup_push(closePipeGlobal, NULL);
    pthread_cleanup_push(waitForAllThreads, NULL);
    SYS(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));




            DEBUG("OK (my thread: %lu)\n", (long unsigned int) pthread_self());
    char buffer[150];

    while(true){
        int resourceType, resourceQuantity, pid;
        DEBUG("Main thread starts reading\n");
        int readBytes = -1;
        CALL(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL), cleanUpThreads(storage));
        while((readBytes = (int) read(pipeDesc, buffer, 120)) == 0) continue;
        CALL(readBytes != 120, finishMain(storage, pipeDesc, true));


        CALL(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL), cleanUpThreads(storage));
//        if (errno > 0) {DEBUG("ERRNO %d\n", errno); finishMain(storage, pipeDesc, true);}
        sscanf(buffer, "%d %d %d", &resourceType, &resourceQuantity, &pid);
        if (resourceType == -1) { cleanUpThreads(storage); break;}
        assignment_received(pid, (size_t) resourceType, (size_t) resourceQuantity);

        DEBUG("Main wanting protection on %d\n", resourceType);
        CALL(pthread_mutex_lock(&storage->resourceProtection[resourceType]), finishMain(storage, pipeDesc, true));
        DEBUG("Protection granted to main on %d\n", resourceType);

//        DEBUG("Read %d %d %d\n", resourceType, resourceQuantity, pid);

        if (storage->firstFromPair[resourceType] == -1){// If first from pair
            storage->firstFromPair[resourceType] = pid;
            storage->resourcesFirstFromPair[resourceType] = resourceQuantity;
            DEBUG("First from pair %d\n", pid);
            CALL(pthread_mutex_unlock(&storage->resourceProtection[resourceType]), finishMain(storage, pipeDesc, true));

        } else{ //Otherwise, create a thread
            struct PassedData* data = malloc(sizeof(struct PassedData));
            if (data == NULL) finishMain(storage, pipeDesc, true);
            data->storage = storage;
            data->resourceType = resourceType;
            data->pid1 = storage->firstFromPair[resourceType];
            data->quantity1 = storage->resourcesFirstFromPair[resourceType];
            data->pid2 = pid;
            data->quantity2 = resourceQuantity;
            data->mainThread = pthread_self();

            storage->resourcesFirstFromPair[resourceType] = storage->firstFromPair[resourceType] = -1;
            CALL(pthread_mutex_unlock(&storage->resourceProtection[resourceType]), finishMain(storage, pipeDesc, true));


            pthread_attr_t attr;
            CALL(pthread_attr_init(&attr), {free(data);finishMain(storage, pipeDesc, true);});
            CALL(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE), {free(data);finishMain(storage, pipeDesc, true);});
            pthread_t newThread;


            CALL(pthread_create(&newThread, &attr, processing, data), {free(data);finishMain(storage, pipeDesc, true);});

            DEBUG("Start thread for pair %d %d (%d): %lu\n", pid,
                    storage->firstFromPair[resourceType], resourceType, (long unsigned int) newThread);


            CALL(pthread_mutex_lock(&storage->listProtection), {free(data); finishMain(storage, pipeDesc, true);});
            addThread(&storage->listOfThreads, newThread);
            CALL(pthread_mutex_unlock(&storage->listProtection), {finishMain(storage, pipeDesc, true);});

            CALL(pthread_attr_destroy(&attr), finishMain(storage, pipeDesc, true));


        }

    }

    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    DEBUG("Main thread ends working\n");
    return 0;
}