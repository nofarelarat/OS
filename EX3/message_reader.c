#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "message_slot.h"


int channelId;
int file_desc;
char * buf;

int main(int argc, char *argv[]) {
    // check that there are 3 arguments
    //0-program name 1-message slot file path, 2-the target message channel id
    assert(argc == 3);
    channelId = atoi(argv[2]);

    int ret_val;

    file_desc = open( argv[1], O_RDWR );
    if( file_desc < 0 )
    {
        printf ("Can't open device file: %s\n",
                DEVICE_FILE_NAME);
        exit(-1);
    }
    buf = malloc (sizeof(char)*(BUF_LEN+1));
    if(buf < 0){
        printf("Error in malloc: %s\n", strerror(errno));

        return -1;
    }
    memset(&buf[0],0,sizeof(buf));

    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, channelId);
    if(ret_val < 0){
        printf("Error in ioctl: %s\n", strerror(errno));
        free(buf);
        return -1;
    }

    ret_val = read(file_desc, buf, BUF_LEN);
    if(ret_val < 0){
        printf("Error in read: %s\n", strerror(errno));
        free(buf);
        return -1;
    }
    buf[ret_val] = '\0';

    close(file_desc);
    printf("the message: %s\n",buf);
    printf( "%d bytes read from %s\n",ret_val,argv[1]);
    free(buf);
    return 0;

}