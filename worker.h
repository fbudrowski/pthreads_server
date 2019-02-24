/**
 * @author Franciszek Budrowski
 * Worker interface: it gets the resources for a pair of clients, waits until these clients have done their work,
 * and returns the resources to the public pool.
 */

#ifndef PTHREADS_POPR_WORKER_H
#define PTHREADS_POPR_WORKER_H

#include "serverStorage.h"


struct PassedData{
    pthread_t mainThread;
    ServerStorage* storage;
    int resourceType;
    int pid1;
    int quantity1;
    int pid2;
    int quantity2;
};


void* processing(void* data0); //(data0 - PassedData)
#endif //PTHREADS_POPR_WORKER_H
