/***
 * Implementation of server's client. This service gets the resources from server, does the work and returns the resources.
 * @author Franciszek Budrowski
 */

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <memory.h>
#include <errno.h>
#include "macros.h"
#include "report_api.h"


char pipeName1Glob[50];
char pipeName2Glob[50];
int openDescriptor = -1;

void onError(){
    DEBUG("ERROR %d\n", errno);
    if (openDescriptor != -1) close(openDescriptor);
    unlink(pipeName1Glob);
    unlink(pipeName2Glob);
    exit(1);
}

void onSigInt() {onError();}

int main(int argv, char** argc){
    openDescriptor = -1;
    if (argv != 4) return 0;
    int k = (int) strtol(argc[1], NULL, 10);
    int n = (int) strtol(argc[2], NULL, 10);
    int p = (int) strtol(argc[3], NULL, 10);

    int pipeDesc;

    char pipeName1[50];
    sprintf(pipeName1, "%s-%d", RESOURCE_ACQUIRED_PIPE, getpid());
    strcpy(pipeName1Glob, pipeName1);
    char pipeName2[50];
    sprintf(pipeName2, "%s-%d", WORK_DONE_PIPE, getpid());
    strcpy(pipeName2Glob, pipeName2);


    CALL(mkfifo(pipeName1, 0666), exit(1));
    CALL(mkfifo(pipeName2, 0666), {unlink(pipeName1); exit(1);});

    struct sigaction saction;
    saction.sa_handler = onError;

    CALL(sigaction(SIGINT, &saction, NULL), onError());



    pipeDesc = open(MAIN_PIPE_NAME, O_WRONLY);
    DEBUG("Pipe opened %d\n", pipeDesc);
    CALL((pipeDesc <= 2), onError());
    openDescriptor = pipeDesc;

    DEBUG("File opened");
    char buffer[150];
    sprintf(buffer, "%d %d %d\n", k, n, getpid());
    CALL(write(pipeDesc, buffer, 120) != 120, onError());
    openDescriptor = -1;
    DEBUG("Wrote to main pipe (%d)\n", getpid());
    CALL(close(pipeDesc), onError());

    pipeDesc = open(pipeName1, O_RDONLY);
    CALL(pipeDesc <= 2, onError());
    openDescriptor = pipeDesc;
    int readBytes = -1;
    while((readBytes = (int) read(pipeDesc, buffer, 120)) == 0) continue;
    if (readBytes != 120) {onError();}
    DEBUG("Read %s\n",buffer);
    int cli1, cli2;
    sscanf(buffer, "%d %d", &cli1, &cli2); // NOLINT(cert-err34-c)
    if (cli1 == -1){
        onSigInt();
    }
    acquired_resources((size_t) k, (size_t) n, (size_t) p, getpid(), (cli1 == getpid()) ? cli2 : cli1);
    openDescriptor = -1;
    CALL(close(pipeDesc), onError());
    CALL(unlink(pipeName1), {unlink(pipeName2);exit(1);});
    DEBUG("Starting work (%d)\n", getpid());

    sleep((unsigned int) (p / 10 + p % 10));

    DEBUG("Work done (%d)\n", getpid());


    pipeDesc = open(pipeName2, O_WRONLY);
    DEBUG("Open desc %d (%d)\n", pipeDesc, getpid());
    CALL(pipeDesc <= 2, {unlink(pipeName2);exit(1);});
    openDescriptor = pipeDesc;
    sprintf(buffer,"%d %d\n", k, n);
    DEBUG("Waiting for write (%d)\n", getpid());
    CALL(write(pipeDesc, buffer, 120) != 120, {unlink(pipeName2);exit(1);});
    DEBUG("Write successful (%d)\n", getpid());
    close(pipeDesc);
    unlink(pipeName2);
    DEBUG("Exit work (%d)\n", getpid());

}
