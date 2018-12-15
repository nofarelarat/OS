#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "message_slot.h"

int cannelId;
int file_desc;
char * buf;

int wfd;
int pid;

int main(int argc, char *argv[]) {
    // check that there are 4 arguments
    //0-program name 1-message slot file path, 2-the target message channel id, 3-the message to pass
    assert(argc == 4);
    cannelId = atoi(argv[2]);

    int ret_val;

    file_desc = open( argv[1], O_RDWR );
    if( file_desc < 0 )
    {
        printf ("Can't open device file: %s\n",
                DEVICE_FILE_NAME);
        return -1;
    }

    buf = malloc (sizeof(char)*(BUF_LEN+1));
    if(buf < 0){
        printf("Error in malloc: %s\n", strerror(errno));

        return -1;
    }
    memset(&buf[0],0,sizeof(buf));
    strcpy(buf, argv[3]);

    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, cannelId);
    if(ret_val < 0){
        printf("Error in ioctl: %s\n", strerror(errno));
        free(buf);
        return -1;
    }

    ret_val = write(file_desc, buf, strlen(buf));
    if(ret_val < 0){
        printf("Error in write: %s\n", strerror(errno));
        free(buf);
        return -1;
    }
    else if (ret_val != strlen(buf)) {
        printf("write msg failed, only %d bytes read.\n", ret_val);
        free(buf);
        exit(-1);
    }

    close(file_desc);
    printf( "%d bytes written to %s\n",ret_val,argv[1]);
    free(buf);
    return 0;
}