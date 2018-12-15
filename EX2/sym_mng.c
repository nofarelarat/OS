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
#include <math.h>
#include <signal.h>

int sumofActiveProcess;
int *processes;
char * symbol;
int *fifofiles;
char in[70];
char filename[20];
int len;


int CloseProcesses(){
    int j;
    for(j=0; j< len; j++)
    {
        if(processes[j] > 0)
        {
            int reskill = kill(processes[j], SIGTERM);
            if (reskill == -1) {
                printf("Error in kill: %s\n", strerror(errno));
                kill(processes[j], SIGKILL);
            }
            close(fifofiles[j]);
            char forunlink[20];
            sprintf(forunlink, "fifofiles/file%d", j);
            unlink(forunlink);
        }
    }
}

int FreeAll() {
    free(processes);
    free(fifofiles);
    free(symbol);
}

int DeletePreviousFifoFiles() {
    char filenameForDelete[20];
    int j;
    for(j=0 ;j<len; j++) {
        sprintf(filenameForDelete, "fifofiles/file%d", j);
        remove(filenameForDelete);
    }
}

void sigpipe_signal_handler_parent()
{
    printf("SIGPIPE for Manager process %d. Leaving.\n", getpid());
    CloseProcesses();
    FreeAll();
    DeletePreviousFifoFiles();
    exit(1);
}

int InitializeProcessArray(char * string, char * filepath)
{
    int j;

    symbol = malloc (sizeof(char));

    for(j=0 ;j<len; j++) {
        *symbol = string[j];
        sprintf(filename, "fifofiles/file%d", j);

        if (mkfifo(filename, S_IRWXU) != 0) {
            printf("Error in mkfifo: %s\n", strerror(errno));
            return -1;
        }


        fifofiles[j] = open(filename, O_RDONLY|O_NONBLOCK);
        if (fifofiles[j] < 0)
        {
            printf("Error in open fifo file: %s\n", strerror(errno));
            return -1;
        }
        char *exec_args[] = {"./sym_count", filepath, symbol , filename , NULL};

        pid_t pid = fork();

        if(pid == -1)
        {
            printf("Error in fork: %s\n", strerror(errno));
            return -1;
        }
        if (pid == 0)
        {
            // Child
            int resexecvp = execvp("./sym_count", exec_args);
            if(resexecvp == -1) {
                printf("Error in exexvp: %s\n", strerror(errno));
                return -1;
            }
        }
        else{
            processes[j]=pid;
        }
    }

    return 1;
}

int main(int argc, char *argv[]) {
    // check that there are 3 arguments
    //0-program name 1-file path, 2-symbols to find
    assert(argc == 3);

    char *string = argv[2];
    len = strlen(string);
    int i;

    struct sigaction SigpipeAction;
    memset(&SigpipeAction, 0, sizeof(SigpipeAction));
    // Assign pointer to our handler function
    SigpipeAction.sa_sigaction = sigpipe_signal_handler_parent;
    if( 0 != sigaction(SIGPIPE, &SigpipeAction, NULL))
    {
        printf("Signal handle registration " "failed. %s\n",
               strerror(errno));
        return -1;
    }

    processes = malloc(sizeof(int) * len);
    memset(&processes[0],0,sizeof(processes));

    fifofiles = malloc(sizeof(int) * len);
    memset(&fifofiles[0],0,sizeof(fifofiles));

    sumofActiveProcess = len;

    int resinit = InitializeProcessArray(string, argv[1]);
    if (resinit == -1) {
        CloseProcesses();
        FreeAll();
        DeletePreviousFifoFiles();
        return -1;
    }


    int status;
    int keeprunning =1;

    while (keeprunning) {
        sleep(1);
        for (i = 0; i < len; i++) {
            if (processes[i] > 0 && fifofiles[i] > 0) {
                int pid = processes[i];
                int waitres = waitpid(pid, &status, WUNTRACED);
                if(waitres == -1){
                    printf("Error in waitpid: %s\n", strerror(errno));

                    CloseProcesses();
                    FreeAll();
                    DeletePreviousFifoFiles();
                    return -1;
                }

                if (WIFEXITED(status) == 1 && waitres == pid)
                {
                    if (read(fifofiles[i], in, sizeof(in)) == -1){
                        printf("Error in read: %s\n", strerror(errno));

                        CloseProcesses();
                        FreeAll();
                        DeletePreviousFifoFiles();
                        return -1;
                    }
                    printf("%s",in);

                    processes[i] = -1;
                    sumofActiveProcess = sumofActiveProcess - 1;
                    close(fifofiles[i]);
                    fifofiles[i] = -1;
                    char forunlink[20];
                    sprintf(forunlink,"fifofiles/file%d",i);
                    unlink(forunlink);

                }
            }
        }
        if(sumofActiveProcess<=0)
        {
            keeprunning =0;
        }
    }

    //all process are finished
    FreeAll();
    DeletePreviousFifoFiles();
    exit(1);
}
