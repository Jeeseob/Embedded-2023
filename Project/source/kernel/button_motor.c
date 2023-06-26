/* FPGA Push Switch Test Application
File : fpga_test_push.c
Auth : largest@huins.com */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <sys/ioctl.h>
#include <signal.h>

#define MAX_BUTTON 9
#define MOTOR_ATTRIBUTE_ERROR_RANGE 4
#define FPGA_STEP_MOTOR_DEVICE "/dev/fpga_step_motor"

int move(int move, int right_left)
{
	int dev;
	unsigned char motor_state[3] = {move, right_left, 50};

	dev = open(FPGA_STEP_MOTOR_DEVICE, O_WRONLY);

	if (dev<0) {
		return -1;
	}

	write(dev,motor_state,3);
	close(dev);
	
	return 1;
}



unsigned char quit = 0;

void user_signal1(int sig) 
{
	quit = 1;
}

int main(void)
{
	int i;
	int dev;
	int buff_size;
	int moved = 0;

	unsigned char push_sw_buff[MAX_BUTTON];

	dev = open("/dev/fpga_push_switch", O_RDWR);

	if (dev<0){
		printf("Device Open Error\n");
		close(dev);
		return -1;
	}

	(void)signal(SIGINT, user_signal1);

	buff_size=sizeof(push_sw_buff);
	printf("Press <ctrl+c> to quit. \n");
	while(!quit){
		usleep(400000);
		read(dev, &push_sw_buff, buff_size);
		if(push_sw_buff[3] == 1 && push_sw_buff[5] == 0)
		{
			moved = 1;
			move(1, 1);
		}
		else if(push_sw_buff[3] == 0 && push_sw_buff[5] == 1)
		{
			moved = 1;
			move(1, 0);
		}
		else if(moved ==1)
		{
			move(0,0);
			moved = 0;
		}
	}
	close(dev);
}
