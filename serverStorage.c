/**
 * Implementation of various storage-based functionalities for server.
 * Including list-based operations, inits and destroy for shared memory.
 * @author Franciszek Budrowski
 */
#include "serverStorage.h"
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bits/fcntl-linux.h>
#include <unistd.h>


bool isEmptyList(LinkedListOfThreads* list){
    return list->first == NULL;
}

LinkedListOfThreads getEmptyList(){
    LinkedListOfThreads list;
    list.first = NULL;
    return list;
}
void addThread(LinkedListOfThreads* list, pthread_t thread){
    Node* node = malloc(sizeof(Node));
    node->thread = thread;
    node->nextNode = list->first;
    list->first = node;
}

void delThread(LinkedListOfThreads* list, pthread_t thread){
    Node* node = list->first, *prevNode = NULL;
    while(node != NULL && node->thread != thread){
        prevNode = node;
        node = node->nextNode;
    }
    if (node != NULL && node->thread == thread){
        if (prevNode != NULL){
            prevNode->nextNode = node->nextNode;
        } else{
            list->first = node->nextNode;
        }
        free(node);
    }
}

void destroyList(LinkedListOfThreads* list){
    Node* node = list->first, *prevNode = NULL;
    while(node != NULL){
        prevNode = node;
        node = node->nextNode;
        free(prevNode);
    }

}



void initStorage(ServerStorage* storage, int resCount, int resEach){

    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);

    SYS(pthread_mutex_init(&storage->listProtection, &mutexattr));

    storage->resourceCount = resCount;
    for (int i = 0; i <= resCount; i++){
        storage->resources[i] = resEach;
        storage->firstFromPair[i] = -1;
        storage->resourcesFirstFromPair[i] = -1;
        storage->waitsForResourcesId[i] = -1;
        storage->waitForHowManyResourceUnits[i] = -1;
        SYS(pthread_mutex_init(&storage->resourceProtection[i], &mutexattr));

        SYS(pthread_cond_init(&storage->waitForResources[i], NULL));
        SYS(pthread_cond_init(&storage->waitForRightToResources[i], NULL));
    }
    storage->listOfThreads = getEmptyList();
    addThread(&storage->listOfThreads, pthread_self());

    pthread_mutexattr_destroy(&mutexattr);
}

void sendFailMessageTo(int pid){
    char pipeName1[50];
    sprintf(pipeName1, "%s-%d", RESOURCE_ACQUIRED_PIPE, pid);
    int fd = open(pipeName1, O_WRONLY);
    if (fd <= 2) {DEBUG("ERROR\n"); exit(1);}
    CALL(write(fd, "-1 0", 100) == -1, exit(1));
    close(fd);
}


void destroyStorage(ServerStorage* storage){
    SYS(pthread_mutex_destroy(&storage->listProtection));
    for(int i = 0;i <= storage->resourceCount; i++){
        if (storage->firstFromPair[i] != -1){
            int pid = storage->firstFromPair[i];
            sendFailMessageTo(pid);
        }

        SYS(pthread_mutex_destroy(&storage->resourceProtection[i]));
        SYS(pthread_cond_destroy(&storage->waitForResources[i]));
    }
    destroyList(&storage->listOfThreads);
    free(storage);
}


void cleanUpThreads(ServerStorage* storage){
    DEBUG("Cleaning up threads\n");
    pthread_mutex_lock(&storage->listProtection);
    Node* node = storage->listOfThreads.first;
    while (node != NULL){
        if (node->thread != pthread_self()){
            pthread_cancel(node->thread);
        }
        node = node->nextNode;
    }
    pthread_mutex_unlock(&storage->listProtection);
    DEBUG("Cancelling myself (%lu)\n", pthread_self());
    pthread_cancel(pthread_self());
    DEBUG("Ordered cancellation of threads\n");
}
