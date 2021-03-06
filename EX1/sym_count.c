//
// Created by root on 3/23/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BUFSIZE 512
int keep_running = 1;
int counter =0;
int fd;
char symbol;
char * buf;

void sigterm_signal_handler()
{
    printf("Process %d finishes. Symbol %c. Instances %d.\n", getpid(), symbol,counter);
    keep_running = 0;// Stop the execution gracefully
    free(buf);
    close(fd); // close file
    exit(1);
}
void sigcont_signal_handler()
{
    printf("Process %d continues", getpid());
}

int main(int argc, char *argv[]) {
    // check that there are 3 arguments
    //0-program name 1-file path, 2-symbol to find
    assert (argc == 3);
    symbol = *argv[2];
    fd = open(argv[1], O_RDONLY);
    if( fd < 0 )
    {
        printf( "Error opening file: %s\n", strerror( errno ) );
        return -1;
    }

    struct sigaction SigtermAction;
    memset(&SigtermAction, 0, sizeof(SigtermAction));
    // Assign pointer to our handler function
    SigtermAction.sa_sigaction = sigterm_signal_handler;
    if( 0 != sigaction(SIGTERM, &SigtermAction, NULL))
    {
        printf("Signal handle registration " "failed. %s\n",
               strerror(errno));
        return -1;
    }
    struct sigaction SigcontAction;
    memset(&SigcontAction, 0, sizeof(SigcontAction));
    // Assign pointer to our handler function
    SigcontAction.sa_sigaction = sigcont_signal_handler;
    if( 0 != sigaction(SIGCONT, &SigcontAction, NULL))
    {
        printf("Signal handle registration " "failed. %s\n",
               strerror(errno));
        return -1;
    }

    int len,j;
    buf = malloc (sizeof(char)*(BUFSIZE+1));
    while (keep_running)
    {
        len = read(fd, buf, BUFSIZE);
        buf[len] = '\0'; // string closer
        for(j=0;j<len;j++)
        {
            if (buf[j] == symbol)
            {
                counter = counter + 1;
                printf("Process %d, symbol %c, going to sleep\n", getpid(), *argv[2]);
                int raiseResult = raise(SIGSTOP);
                if(raiseResult != 0){
                    free (buf);
                    printf("Error in raise: %s\n", strerror(errno));
                    return -1;
                }
            }
        }

        if (len < 0) {
            printf("Error reading from file: %s\n", strerror(errno));
            free(buf);
            return -1;
        }
        if (len == 0) {
            //"EOF is reached
            int raiseResult = raise(SIGTERM);
            if(raiseResult != 0){
                free (buf);
                printf("Error in raise: %s\n", strerror(errno));
                return -1;
            }
        }
    }

    free(buf);
    close(fd); // close file
    exit(1);
}

                                                                                                                                                                                                                                                                                                                                                      