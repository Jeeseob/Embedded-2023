/* Test Application
File : test_dev_driver.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "ioctl.h"

#define DEV_DEVICE "/dev/dev_driver"
#define MAX_DIGIT       4


int main(int argc, char **argv)
{
    int i;
	int dev;
    int str_size;
	int data;
    int count;
    struct ioctl_info value;

	if(argc!=4) {
		printf("please input the parameter! \n");
		printf("ex)./test_led a1\n");
		return -1;
	}

	data = atoi(argv[1]);
	if((data < 1)||(data > 100))
	{
		printf("[argv1 ERROR]Invalid range!\n");
        exit(1);
    }
    value.timer_interval = data;

    data = atoi(argv[2]);
	if((data < 1)||(data > 100))
	{
		printf("[argv2 ERROR]Invalid range!\n");
        exit(1);
    }
    value.timer_cnt = data;

    str_size=strlen(argv[3]);
    if(str_size>MAX_DIGIT)
    {
        printf("Warning! 4 Digit number only!\n");
        str_size=MAX_DIGIT;
    }

    for(i=0;i<str_size;i++)
    {
        if((argv[3][i]<0x30)||(argv[3][i])>0x38) {
            printf("[argv3 ERROR] Invalid Value!\n");
            return -1;
        }
        value.timer_char[i] = argv[3][i] - 0x30;
    }
    // 3 zero 
    count = 0;
    for(i=0; i<str_size; i++) {
        if(value.timer_char[i] == 0) {
            count++;
        }
    }

    if(count != 3) {
        printf("[argv3 ERROR] Invalid Value!!\n");
        return -1;
    }

    dev = open(DEV_DEVICE, O_RDWR);
    if (dev<0) {
        printf("Device open error : %s\n",DEV_DEVICE);
        exit(1);
    }

    if(ioctl(dev, SET_OPTIONS, &value) < 0) {
        perror("[IOCTL ERROR] SET_OPTIONS Error\n");
        close(dev);
        return -1;
    }

    sleep(1);

    if(ioctl(dev, START_TIMER) < 0) {
        perror("[IOCTL ERROR] START_TIMER Error\n");
        close(dev);
        return -1;
    }

    printf("\n");
	close(dev);

	return(0);
}
