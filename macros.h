/**
 * @author Franciszek Budrowski
 */

#ifndef PTHREADS_POPR_MACROS_H
#define PTHREADS_POPR_MACROS_H

#include<stdio.h>

#define MAIN_PIPE_NAME "mainPipe"
#define RESOURCE_ACQUIRED_PIPE "resourceAcquiredPipe"
#define WORK_DONE_PIPE "workDonePipe"



#define MAX_RESOURCES 111

#define CALL(system, action) if ((system) != 0) {fprintf(stderr, "ERROR(%lu)\n", (long unsigned int)pthread_self()); action;exit(1);}
#define SYS(system) if((system) != 0) {fprintf(stderr, "ERROR(%lu)\n", (long unsigned int)pthread_self); exit(1);}
#define DEBUG(args...) //printf(args) //If you want to use debug, uncomment printf.



#endif //PTHREADS_POPR_MACROS_H
