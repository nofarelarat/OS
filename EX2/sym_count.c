//
// Created by root on 4/14/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>


int counter =0;
char symbol;
int fd;
char *shared;
int size;
int wfd;
int pid;

void sigpipe_signal_handler_child()
{
    printf("SIGPIPE for process %d. Symbol %c. Counter %d. Leaving.\n", pid, symbol,counter);

    int raiseResult = raise(SIGTERM);
    if(raiseResult != 0)
    {
        printf("Error in raise: %s\n", strerror(errno));
        if (munmap(shared, size) == -1)
            printf("Child failed un-mapping file: %s\n", strerror(errno));
        close(fd);
        close(wfd);
        exit(0);
    }
}

void sigterm_signal_handler_child()
{
    if (munmap(shared, size) == -1) {
        printf("Child failed un-mapping file: %s\n", strerror(errno));
        close(fd);
        close(wfd);
        exit(0);
    }
    close(fd);
    close(wfd);
    exit(1);
}

int main(int argc, char *argv[]) {
    // check that there are 4 arguments
    //0-program name 1-file path, 2-symbol to find, 3-fifofile
    assert(argc == 4);
    symbol = *argv[2];
    pid = getpid();

    struct sigaction SigpipeAction;
    memset(&SigpipeAction, 0, sizeof(SigpipeAction));
    // Assign pointer to our handler function
    SigpipeAction.sa_sigaction = sigpipe_signal_handler_child;
    if( 0 != sigaction(SIGPIPE, &SigpipeAction, NULL))
    {
        printf("Signal handle registration " "failed. %s\n",
               strerror(errno));
        return -1;
    }

    struct sigaction SigtermAction;
    memset(&SigtermAction, 0, sizeof(SigtermAction));
    // Assign pointer to our handler function
    SigtermAction.sa_sigaction = sigterm_signal_handler_child;
    if( 0 != sigaction(SIGTERM, &SigtermAction, NULL))
    {
        printf("Signal handle registration " "failed. %s\n",
               strerror(errno));
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd == -1)
    {
        printf("Child failed opening file: %s\n", strerror(errno));
        return -1;
    }

    struct stat buf;
    fstat(fd, &buf);
    size = buf.st_size;

    // Create a string pointing to the shared memory
    shared = (char *) mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0);

    if (shared == MAP_FAILED) {
        printf("Failed mapping file to memory: %s\n", strerror(errno));
        close(fd);
        return  -1;
    }
    int i;
    for (i = 0; i < size; i++)
    {
        char c;
        c = shared[i];

        if (c == symbol)
        {
            counter = counter + 1;
        }
    }
    char out[70];// max int + return message
    sprintf(out, "Process %d finishes. Symbol %c. Instances %d.\n", pid,symbol,counter);

    if ((wfd = open(argv[3], O_WRONLY)) < 0)
    {
        printf("Child failed opening file: %s\n", strerror(errno));

        if (munmap(shared, size) == -1)
        {
            printf("Child failed un-mapping file: %s\n", strerror(errno));
        }
        close(fd); // close file
        return -1;
    }

    if (write(wfd, out, strlen(out)+1) == -1){
        printf("Child failed writing file: %s\n", strerror(errno));

        if (munmap(shared, size) == -1)
        {
            printf("Child failed un-mapping file: %s\n", strerror(errno));
        }
        close(fd); // close file
        close(wfd);
        return -1;
    }

    int raiseResult = raise(SIGTERM);
    if(raiseResult != 0)
    {
        printf("Error in raise: %s\n", strerror(errno));
        if (munmap(shared, size) == -1)
            printf("Child failed un-mapping file: %s\n", strerror(errno));
        close(fd);
        close(wfd);
        return  -1;
    }
    exit(1);
}
