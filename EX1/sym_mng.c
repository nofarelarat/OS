//
// Created by root on 3/24/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

int sumofActiveProcess;
int *processes;
int *stopCounters;
char * symbol;

int InitializeProcessArray(int len, char * string, char * filepath)
{
    int j;
    symbol = malloc (sizeof(char));
    for(j=0 ;j<len; j++)
    {
        *symbol = string[j];

        char *exec_args[] = {"./sym_count.out", filepath, symbol , NULL};
        pid_t pid = fork();

        if(pid == -1)
        {
            printf("Error in fork: %s\n", strerror(errno));
            free(processes);
            free(stopCounters);
            free(symbol);
            return -1;
        }
        if (pid == 0)
        {
            // Child
            processes[j]=getpid();
            int res = execvp("./sym_count.out", exec_args);
            if(res == -1) {
                printf("Error in exexvp: %s\n", strerror(errno));
                free(processes);
                free(stopCounters);
                free(symbol);
                return -1;
            }
        }
    }
    sleep(1);
    return 1;
}

int InitializeCountersArray(int len){
    int j;
    for(j=0; j< len; j++){
        stopCounters[j] = 0;
    }
}

int main(int argc, char *argv[]) {
    // check that there are 4 arguments
    //0-program name 1-file path, 2-symbols to find, 3-Termination bound
    assert(argc == 4);
    int i, TerminationBound, killRes;
    char *string = argv[2];
    int len = strlen(string);

    processes = malloc(sizeof(int) * len);
    //-1 in cell - not a managed process

    stopCounters = malloc(sizeof(int) * len);

    //initialize arrays
    int res = InitializeProcessArray(len, string, argv[1]);

    if (res == -1) {
        return -1;
    }
    InitializeCountersArray(len);

    TerminationBound = atoi(argv[3]);

    sumofActiveProcess = len;

    int status;
    while (sumofActiveProcess > 0) {
        for (i = 0; i < len; i++) {
            if (processes[i] >= 0) {
                int pid = processes[i];
                waitpid(pid, &status, WUNTRACED);
                if (WIFSTOPPED(status) == 1)
                {
                    stopCounters[i] = stopCounters[i] + 1;
                    if (stopCounters[i] == TerminationBound)
                    {
                        killRes = kill(pid, SIGTERM);
                        if (killRes == -1) {
                            printf("Error in kill: %s\n", strerror(errno));
                            free(processes);
                            free(stopCounters);
                            free(symbol);
                            return -1;
                        }
                        killRes = kill(pid, SIGCONT);
                        if (killRes == -1) {
                            printf("Error in kill: %s\n", strerror(errno));
                            free(processes);
                            free(stopCounters);
                            free(symbol);
                            return -1;
                        }

                        processes[i] = -1;
                        sumofActiveProcess = sumofActiveProcess - 1;
                    }
                    else if (stopCounters[i] < TerminationBound)
                    {
                        kill(pid, SIGCONT);
                        if (killRes == -1) {
                            printf("Error in kill: %s\n", strerror(errno));
                            free(processes);
                            free(stopCounters);
                            free(symbol);
                            return -1;
                        }
                    }
                }
                if (WIFEXITED(status) == 1) {
                    processes[i] = -1;
                    sumofActiveProcess = sumofActiveProcess - 1;
                }
            }
        }
    }
    free(processes);
    free(stopCounters);
    free(symbol);
    //all process are finished
    int raiseResult = raise(SIGTERM);
    if (raiseResult != 0) {
        printf("Error in raise: %s\n", strerror(errno));
        return -1;
    }
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         