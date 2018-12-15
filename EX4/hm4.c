//
// Created by root on 5/19/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


#define BUFSIZE 1024*1024
int fd_out;
char * shared_buf;
pthread_t* thread_ids;
int num_threads;
int file_num;
int total_len;
int shared_buf_pos = 0;
int global_count = 0;
int *is_finished;
int is_last_one = 1;
pthread_mutex_t mutex;
pthread_cond_t cond;

void *thread_func(void *thread_param){
    printf("in thread_func\n");
    int j, k, len, fd_in, keep_running=1, local_count=0;
    char data;
    char * buf;

    fd_in = open(thread_param, O_RDONLY);
    if( fd_in < 0 ) {
        printf( "Error opening file: %s\n", strerror( errno ) );
        return (void *)-1;
    }
    buf = malloc (sizeof(char)*(BUFSIZE+1));
    memset(&buf[0],0,sizeof(buf));

    while(keep_running){
        len = read(fd_in, buf, BUFSIZE);

        if (len < 0) {
            printf("Error reading from file: %s\n", strerror(errno));
            free(buf);
            close(fd_in);
            return (void *)-1;
        }
        if (len < BUFSIZE) {
            //EOF is reached
            keep_running = 0;// Stop the execution gracefully
            is_finished[global_count] = 1;
            global_count++;
            free(buf);
            close(fd_in);
        }
        local_count++;
        int res = pthread_mutex_lock(&mutex);
        if(res != 0){
            printf("error in pthread_mutex_lock\n");
            free(buf);
            close(fd_in);
            return (void *)-1;
        }

        //data = buf[0];

        while(local_count>global_count){
            res = pthread_cond_wait(&cond, & mutex);
            if(res != 0){
                printf("error in pthread_cond_wait\n");
                free(buf);
                close(fd_in);
                return (void *)-1;
            }
        }
        for(j=0;j<len;j++){
            //data = data^buf[j];
            //if(BUFSIZE*local_count + j >= shared_buf_pos){
            if(j >= shared_buf_pos){
                //shared_buf[BUFSIZE*local_count + j] = buf[j];
                shared_buf[j] = buf[j];
                shared_buf_pos++;
            }
            else {
                //shared_buf[BUFSIZE*local_count + j] = shared_buf[BUFSIZE*local_count + j] ^ buf[j];
                shared_buf[j] = shared_buf[j] ^ buf[j];
                //shared_buf_pos++;
            }
        }

        //shared_buf[shared_buf_pos] = data;

        //shared_buf_pos++;

        for(k=0 ;k<num_threads; k++){
            if(is_finished[k] == 0)
                is_last_one = 0;
        }
        if(is_last_one == 1) {
            len = write(fd_out, shared_buf, BUFSIZE);
            if (len <= 0) {
                printf("Error writing to file: %s\n", strerror(errno));
                free(shared_buf);
                close(fd_out);
                return (void *) -1;
            }
            shared_buf_pos = 0;
            total_len = total_len + len;
            memset(&shared_buf[0], 0, sizeof(shared_buf));
        }
        is_last_one = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        memset(&buf[0], 0, sizeof(buf));
    }

    //if(keep_running == 0)
    //{
    //    for(k=0 ;k<num_threads; k++){
    //        if(is_finished[k] == 0)
    //            is_last_one = 0;
    //fdksjhyfiudsfyu
    //    }
    //    if(is_last_one == 1) {
    //        len = write(fd_out, shared_buf, BUFSIZE);
    //        if (len <= 0) {
    //            printf("Error writing to file: %s\n", strerror(errno));
    //            free(shared_buf);
    //            close(fd_out);
    //            return (void *) -1;
    //        }
    //        total_len = total_len + len;
    //        memset(&buf[0], 0, sizeof(shared_buf));
    //    }
    //    is_last_one = 1;
    //    pthread_cond_signal(&cond);
    //    pthread_mutex_unlock(&mutex);
    //}
    //close(fd_out);
}

int Initialize(char *argv[]){
    int i;
    file_num=2;
    thread_ids = malloc(sizeof(pthread_t)*(num_threads));
    for(i=0;i<num_threads;i++)
    {
        int rc;
        long t = 1;

        rc = pthread_create(&thread_ids[i], NULL, thread_func, argv[i+2]);
        file_num++;
        if (rc)
        {
            printf("ERROR in pthread_create(): %s\n", strerror(rc));
            free(thread_ids);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    // check that there are at least 3 arguments
    //0-program name, 1-name of the output file, the rest of the arguments are the names of the input files.
    assert(argc >= 3);
    printf("Hello, creating %s from %d input files\n", argv[1], argc-2);
    num_threads = (argc-2);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    fd_out = open(argv[1], O_TRUNC);
    //fd_out = open(argv[1], O_CREAT|O_WRONLY, 0777);
    if( fd_out < 0 ) {
        printf( "Error opening file: %s\n", strerror( errno ) );
        return -1;
    }

    shared_buf = malloc (sizeof(char)*(BUFSIZE));
    is_finished = malloc (sizeof(int)*(num_threads));
    memset(&shared_buf[0],0,sizeof(shared_buf));
    memset(&is_finished[0],0,sizeof(is_finished));

    int init_res = Initialize(argv);
    if(init_res < 0){
        free(shared_buf);
        free(is_finished);
        return -1;
    }


    for (int i = 0; i < num_threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    pthread_exit(NULL);
    free(shared_buf);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    int res = close(fd_out); // close file
    if(res == -1){
        printf( "Error closing file: %s\n", strerror( errno ) );
        return -1;
    }
    printf( "Created %s with size %d bytes", argv[1], total_len);
    exit(0);
}