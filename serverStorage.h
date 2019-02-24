/**
 * Definition of shared memory (between threads) server storage, including an interface for a list of (p)threads.
 * @author Franciszek Budrowski
 */
#ifndef PTHREADS_POPR_SERVERSTORAGE_H
#define PTHREADS_POPR_SERVERSTORAGE_H

#include <sys/types.h>
#include <malloc.h>
#include <stdbool.h>
#include "macros.h"



typedef struct Node Node;
struct Node{
    pthread_t thread;
    Node* nextNode;
};
typedef struct LinkedListOfThreads LinkedListOfThreads;
struct LinkedListOfThreads{
    Node* first;
};

bool isEmptyList(LinkedListOfThreads* list);

LinkedListOfThreads getEmptyList();
void addThread(LinkedListOfThreads* list, pthread_t thread);

void delThread(LinkedListOfThreads* list, pthread_t thread);



struct ServerStorage{
    int resourceCount;
    pthread_mutex_t resourceProtection[MAX_RESOURCES];
    int resources[MAX_RESOURCES];

    int  firstFromPair[MAX_RESOURCES];
    int resourcesFirstFromPair[MAX_RESOURCES];

    int waitsForResourcesId[MAX_RESOURCES];
    int waitForHowManyResourceUnits[MAX_RESOURCES];


    pthread_cond_t waitForResources[MAX_RESOURCES];

    pthread_cond_t waitForRightToResources[MAX_RESOURCES];

    pthread_mutex_t listProtection;
    LinkedListOfThreads listOfThreads;

};
typedef struct ServerStorage ServerStorage;

void sendFailMessageTo(int pid);

void initStorage(ServerStorage* storage, int resCount, int resEach);

void destroyStorage(ServerStorage* storage);
void cleanUpThreads(ServerStorage* storage);

#endif //PTHREADS_POPR_SERVERSTORAGE_H
