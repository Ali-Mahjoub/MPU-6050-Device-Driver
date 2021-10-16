#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/ioctl.h>
 

int main()
{
        int fd;
        int8_t read_buf[1024];
        printf("\nOpening Driver\n");
        fd = open("/dev/myDeviceNr", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }
 
        
        printf("Writing SENSOR DATA\n");
        read(fd, read_buf, 1024);
        printf("Data = %s\n\n", read_buf);
 
 
        printf("Closing Driver\n");
        close(fd);
}