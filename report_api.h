/*
 * This reporting API was a part of the assignment, and its implementation was included in the task.
 * Credits (for the Polish-language version) to the Uni of Warsaw
 *
 */

#ifndef RAPORT_API_H_
#define RAPORT_API_H_

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>


/*** Functions to be caled by the server ***/

/**
 * Reports readiness to serve clients. After this function is called, any request must be accommodated.
 */
void server_ready(pid_t server_pid);

/**
 * Reports having received a request for resource_count resource_types from the client.
 * Called directly after obtaining a request, before processing that request.
 *
 */
void assignment_received(pid_t client_pid, size_t resource_type, size_t resource_count);

/**
 * Reports assignment of resources to a pair of clients.
 * Server should call this function before notifying clients that the resources have been assigned to them.
 */
void allocated_resources(pthread_t thread, size_t client_1_resources,
                         size_t client_2_resources, size_t resource_type, pid_t client_1,
                         pid_t client_2, size_t resources_remaining);

/**
 * Reports return of resources.
 */
void resources_available(size_t resource_type, size_t resource_amount);




/*** Function called by the client: ***/

/**
 * Reports having acquired resources.
 */
void acquired_resources(size_t resource_type, size_t resource_amount,
                        size_t work_type, pid_t client, pid_t partner);


#endif /* RAPORT_API_H_ */