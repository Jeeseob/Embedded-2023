#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
int main(void){
	int fd;
	int retn;
	char buf[2] = {0,};

	fd = open("/dev/stopwatch", O_RDWR);
	if(fd < 0) {
		perror("/dev/stopwatch");
		exit(-1);
	}
        else { printf("< inter Device has been detected > \n"); }
	
	retn = write(fd, buf, 2);
	close(fd);

	return 0;
}
