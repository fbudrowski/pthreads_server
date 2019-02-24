/*
 * Sample implementation of reporting library. Credits (for the Polish-language version) to University of Warsaw.
 *
 */

#include "report_api.h"

#include <stdio.h>


void server_ready(pid_t server_pid) {
    printf("Server %u is ready\n",
           server_pid);
}

void assignment_received(pid_t client_pid, size_t resource_type, size_t resource_count) {
    printf("Client %u requests %zu units of resource %zu\n",
           client_pid, resource_count, resource_type);
}

void allocated_resources(pthread_t thread, size_t client_1_resources,
                         size_t client_2_resources, size_t resource_type, pid_t client_1,
                         pid_t client_2, size_t resources_remaining) {

    printf("Thread %lu assigns %zu + %zu resources of type %zu to clients %u i %u, and %zu resources remain\n",
           thread, client_1_resources, client_2_resources, resource_type, client_1, client_2,
           resources_remaining);
}

void resources_available(size_t resource_type, size_t resource_amount) {
    printf("%zu resources of type %zu available again\n",
           resource_amount, resource_type);
}


void acquired_resources(size_t resource_type, size_t resource_amount,
                        size_t work_type, pid_t client, pid_t partner) {

    printf("Client %u: received %zu resources of type %zu to do the work %zu. Partner: %u\n",
           client, resource_amount, resource_type, work_type, partner);
}