/**
 * @author Franciszek Budrowski
 * Worker implementation
 */


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "worker.h"
#include "serverStorage.h"
#include "report_api.h"


void deleteThreadFromList(ServerStorage *storage);

void sendResAcquiredAnotherPid(int pid1, int pid2);

void getResources(ServerStorage *storage, int resourceType, int pid1, int quantity1, int pid2, int quantity2);

void getConfirmation(int pid1);

void freeResources(ServerStorage *storage, int resourceType, int quantity1, int quantity2);

void finishLocal(){
    DEBUG("SIGINT on child\n");
    pthread_exit(0);
}

void restoreResources(void* data0){
    struct PassedData data = *(struct PassedData*)data0;
    freeResources(data.storage, data.resourceType, data.quantity1, data.quantity2);
}

void* processing(void* data0){
    CALL(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL), exit(1));
    struct PassedData data = *(struct PassedData*)data0;
    free(data0);
    ServerStorage* storage = data.storage;
    int resourceType = data.resourceType;
    int pid1 = data.pid1;
    int quantity1 = data.quantity1;
    int pid2 = data.pid2;
    int quantity2 = data.quantity2;
    DEBUG("Starting thread with %d, %d\n", pid1, pid2);

    getResources(storage, resourceType, pid1, quantity1, pid2, quantity2);


    pthread_cleanup_push(sendFailMessageTo, &pid1);
    pthread_cleanup_push(sendFailMessageTo, &pid2);
    pthread_cleanup_push(restoreResources, &data);
    CALL(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL), exit(1));
    CALL(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL), exit(1));
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);


    sendResAcquiredAnotherPid(pid1, pid2);
    sendResAcquiredAnotherPid(pid2, pid1);


    DEBUG("Exiting with resources %d %d + %d, (%d, %d)\n", resourceType, quantity1, quantity2, pid1, pid2);
    getConfirmation(pid1);
    getConfirmation(pid2);


    DEBUG("Freeing resources %d,  %d + %d (%d, %d)\n", resourceType, quantity1,  quantity2, pid1, pid2);

    freeResources(storage, resourceType, quantity1, quantity2);


    for(int i = 0; i <= storage->resourceCount; i++){
        DEBUG("Remaining %d resources of type %d\n", storage->resources[i], i);
    }

    deleteThreadFromList(storage);
    return NULL;
}

void freeResources(ServerStorage *storage, int resourceType, int quantity1, int quantity2) {
    DEBUG("Free resources function %d (%d + %d) (%lu)\n", resourceType, quantity1, quantity2, (unsigned long int) pthread_self());

    CALL(pthread_mutex_lock(&storage->resourceProtection[resourceType]), finishLocal());
    DEBUG("Free resources function got access rights %d\n", resourceType);
    storage->resources[resourceType] += quantity1 + quantity2;
    CALL(pthread_mutex_unlock(&storage->resourceProtection[resourceType]), finishLocal());
    DEBUG("Free resources function returned access rights %d\n", resourceType);
    

    CALL(pthread_cond_signal(&storage->waitForResources[resourceType]), finishLocal());
    CALL(pthread_cond_signal(&storage->waitForRightToResources[resourceType]), finishLocal());
    resources_available((size_t) resourceType, (size_t) (quantity1 + quantity2));
}

void getConfirmation(int pid1) {
    DEBUG("Awaiting confirmation from %d\n", pid1);
    char pipeName2[50];
    sprintf(pipeName2, "%s-%d", WORK_DONE_PIPE, pid1);
    int fd = open(pipeName2, O_RDONLY);
    CALL(fd <= 2, finishLocal());
    char text[130]; int val = -1;
    while((val = (int) read(fd, text, 120)) == 0) continue;
    CALL(val != 120, {close(fd);finishLocal();});
    if (text[0] == 0) {
        DEBUG("ERROR conf from %d\n", pid1);
        close(fd);
        return;
    }
    close(fd);
    DEBUG("Got confirmation from %d\n", pid1);

}

void getResources(ServerStorage *storage, int resourceType, int pid1, int quantity1, int pid2, int quantity2) {
    DEBUG("Get access for %d (%lu)\n", resourceType, (long unsigned int) pthread_self());
    CALL(pthread_mutex_lock(&storage->resourceProtection[resourceType]), finishLocal());
    DEBUG("Gotten access for %d\n", resourceType);

    while(storage->waitsForResourcesId[resourceType] != -1){
        CALL(pthread_cond_wait(&storage->waitForRightToResources[resourceType], &storage->resourceProtection[resourceType])
        , finishLocal());
    }
    storage->waitsForResourcesId[resourceType] = pid1;
    storage->waitForHowManyResourceUnits[resourceType] = quantity1 + quantity2;
    DEBUG("Player %lu starts waiting for its resources\n", pthread_self());

    while (storage->resources[resourceType] < quantity1 + quantity2){
        CALL(pthread_cond_wait(&storage->waitForResources[resourceType], &storage->resourceProtection[resourceType]),
        finishLocal());
    }
    storage->waitsForResourcesId[resourceType] = -1;
    storage->waitForHowManyResourceUnits[resourceType] = 0;

    storage->resources[resourceType] -= quantity1 + quantity2;
    CALL(pthread_mutex_unlock(&storage->resourceProtection[resourceType]), finishLocal());

    allocated_resources(pthread_self(), (size_t) quantity1, (size_t) quantity2, (size_t) resourceType, pid1, pid2,
                        (size_t) storage->resources[resourceType]);

    CALL(pthread_cond_signal(&storage->waitForRightToResources[resourceType]), finishLocal());
    CALL(pthread_cond_signal(&storage->waitForResources[resourceType]), finishLocal());



}

void sendResAcquiredAnotherPid(int pid1, int pid2) {
    char pipeName1[50];
    sprintf(pipeName1, "%s-%d", RESOURCE_ACQUIRED_PIPE, pid1);
    int fd = open(pipeName1, O_WRONLY);
    CALL(fd <= 2, {exit(1);});
    char text[130];
    sprintf(text, "%d %d\n", pid1, pid2);
    CALL(write(fd, text, 120) != 120, {close(fd); exit(1);});
    close(fd);
}

void deleteThreadFromList(ServerStorage *storage) {
    CALL(pthread_mutex_lock(&storage->listProtection), finishLocal());
    delThread(&storage->listOfThreads, pthread_self());
    CALL(pthread_mutex_unlock(&storage->listProtection), finishLocal());
}

