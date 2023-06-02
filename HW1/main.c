#include<sys/shm.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h> // for using thread
#include <errno.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>

// for FND
#define MAX_DIGIT 4
#define FND_DEVICE "/dev/fpga_fnd"
volatile unsigned char fnd_data[4];

// for text LCD
#define MAX_BUFF 32
#define LINE_BUFF 16
#define FPGA_TEXT_LCD_DEVICE "/dev/fpga_text_lcd"

// for switch
#define MAX_BUTTON 9

int keyValue_flag;

// for Motor
#define MOTOR_ATTRIBUTE_ERROR_RANGE 4
#define FPGA_STEP_MOTOR_DEVICE "/dev/fpga_step_motor"

// for ready key
#define BUFF_SIZE 64
#define KEY_RELEASE 0
#define KEY_PRESS 1

// for LED Device control
#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16 

// for key value store
# define MAX_KEY_LENGTH 5
# define MAX_VALUE_LENGH 16
# define MAX_MEM_SIZE 3

volatile int back_ground_flag;

typedef struct {
    int index;
    unsigned char key[MAX_KEY_LENGTH];
    unsigned char value[MAX_VALUE_LENGH];
} keyValue;

// for IPC shared memory
# define SHM_SIZE_KEY 4
# define SHM_SIZE_VALUE 16
# define SHM_SIZE_INTEGER sizeof(int)
# define SHM_SIZE_KEY_VALUE sizeof(keyValue)
# define SHM_CREATE 0666


// for shared memroy
int put_key; 
int put_value;
int get_key;
int get_key_value;
int ipc_mode_flag;
int ipc_merge;
int ipc_exit;
int ipc_motor_flag;
int ipc_read_get_key_value_flag;
int ipc_exit_merge;
int ipc_find_order;

unsigned char* put_key_addr;
unsigned char* put_value_addr;
unsigned char* get_key_addr;

keyValue* get_key_value_addr;
int* ipc_mode_flag_addr;
int* ipc_merge_addr;
int* ipc_exit_addr;
int* ipc_motor_flag_addr;
int* ipc_read_get_key_value_flag_addr;
int* ipc_exit_merge_addr;
int* ipc_find_order_addr;

// for Key value
int memTable_pointer;
keyValue* memTable;
int mem_index;
volatile int data_index;
// led flag
volatile int led_flag;

// ready key flag
volatile int exit_flag;
volatile int input_flag;


/**
 * Key Value Store functions 
 */

int get_file_count() {
    DIR *dir;
    struct dirent *entry;
    char *filename;
    int max = 0;
    int file_count = 0;

    printf("get files\n");
    dir = opendir("./");
    if (dir == NULL) {
        printf("Error opening directory: %s\n", strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        filename = entry->d_name;
        char* dot =  strrchr(filename, '.');
        if(dot != NULL) {
            if (strcmp(".st", dot) == 0) { // chzck if the file extension matches
                file_count++;
            }
        }
    }
    printf("file count : %d\n", file_count);
    *ipc_merge_addr = file_count; // count nuber of file
    closedir(dir);
    return file_count;
}

/**
 * init
*/

int get_max_filename(char *extension) {
    DIR *dir;
    struct dirent *entry;
    char *filename;
    int max = 0;
    int file_count = 0;

    printf("get files\n");
    dir = opendir("./");
    if (dir == NULL) {
        printf("Error opening directory: %s\n", strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        filename = entry->d_name;
        char* dot =  strrchr(filename, '.');
        if(dot != NULL) {
            printf("search : %s\n", dot);
            if (strcmp(extension, dot) == 0) { // chzck if the file extension matches
                int val = atoi(filename);
                if (val > max) {
                    max = val;
                }
                file_count++;
            }
        }
    }
    *ipc_merge_addr = file_count; // count nuber of file
    closedir(dir);
    return max;
}


int get_merge_data(FILE* fp, keyValue* merge_data, int index_data) {
    char line[256];
    char* token;
    while (fgets(line, 256, fp) != NULL) {
        printf("line : %s", &line);
        // 개행 문자 제거
        line[strcspn(line, "\n")] = 0;

        // 문자열을 리하여 구조체 멤버에 저장
        token = strtok(line, " ");
        merge_data[index_data].index = atoi(token);

        token = strtok(NULL, " ");
        strcpy(merge_data[index_data].key, token);

        int i;
        token = strtok(NULL, " ");
        for (i = 0; i < strlen(token); i++) {
            merge_data[index_data].value[i] = token[i];
        }
        merge_data[index_data].value[strlen(token)] = '\0';
        printf("(%d, %s, %s)\n", merge_data[index_data].index, &merge_data[index_data].key, &merge_data[index_data].value);
        index_data++;
    }
    return index_data;
}

int merge_and_sort(keyValue* merge_data, int numberOfData){
    int i, j;
    // key가 같은 경우 index가 큰 것만 남기고 삭제
    for (i = 0; i < numberOfData; i++) {
        for (j = i + 1; j < numberOfData; j++) {
            if (strcmp(merge_data[i].key, merge_data[j].key) == 0) {
                if (merge_data[i].index < merge_data[j].index) {
                    merge_data[j].index = -1;
                } else {
                    merge_data[i].index = -1;
                }
            }
        }
    }

    // 정렬하기 전에 index가 -1인 레코드 제거
    int temp_num = 0;
    for (i = 0; i < numberOfData; i++) {
        if (merge_data[i].index != -1) {
            if (temp_num != i) {
                merge_data[temp_num] = merge_data[i];
            }
            temp_num++;
        }
    }

    // 남은 레코드를 key를 기준으로 오름차순으로 정렬
    keyValue temp;
    for (i = 0; i < temp_num-1; i++) {
        for (j = 0; j < temp_num - i-1; j++) {
            if (strcmp(merge_data[j].key, merge_data[j + 1].key) > 0) {
                // swap
                temp = merge_data[j];
                merge_data[j] = merge_data[j + 1];
                merge_data[j + 1] = temp;
            }
        }
    }

    // index 다시 맞추기
    for (i = 0; i < temp_num; i++) {
        merge_data[i].index = i + 1;
    }

    return temp_num;
}

int merge(){
    keyValue merge_data[1024];
    int temp_file_number = get_max_filename(".st")+1; // exclude that file name;
    
    char filename1[32];
    char filename2[32];
    char merge_filename[32];
    sprintf(merge_filename, "./%d.st", temp_file_number);
    if(back_ground_flag == 1) {
        sprintf(filename1, "./%d.st", temp_file_number-2);
        sprintf(filename2, "./%d.st", temp_file_number-3);
    }
    else {
        sprintf(filename1, "./%d.st", temp_file_number-1);
        sprintf(filename2, "./%d.st", temp_file_number-2);
    }
    printf("merge_file : %s\n", merge_filename);
    printf("file1 : %s\n", filename1);
    printf("file2 : %s\n", filename2);


    FILE* file_1 = fopen(filename1, "r");
    if (file_1 == NULL) {
        return 0;
    }    
    FILE* file_2 = fopen(filename2, "r");
    if (file_2 == NULL) {
        return 0;
    }

    int index = get_merge_data(file_2, merge_data, get_merge_data(file_1, merge_data, 0));
    // merge_data merge and sort
    index = merge_and_sort(merge_data, index);

    FILE* file_3 = fopen(merge_filename, "wr+");
    if (file_1 == NULL) {
        return 0;
    }

    int i;
    for(i = 0; i< index; i++){
        if(merge_data[i].index == 0) {
            break;
        }
        fprintf(file_3, "%d %s %s\n", merge_data[i].index, &merge_data[i].key, &merge_data[i].value);
    }
    if(get_file_count() == 1) {
        *ipc_find_order_addr = 1;
    }
    else {
        *ipc_find_order_addr = 0;
    }


    fclose(file_1);
    fclose(file_2);
    printf("delte file\n");
    remove(filename1);
    remove(filename2);
    fclose(file_3);
    back_ground_flag = 0;
    return 1;
}

int write_to_memory(keyValue data) {
    printf("write to memory\n");
    memTable[mem_index].index = data.index;
    strcpy(memTable[mem_index].key, data.key);

    if(strlen(data.value) == 0) {
        strcpy(memTable[mem_index].value, "[empty]");
    }
    else {
        strcpy(memTable[mem_index].value, data.value);
    }
    mem_index++;
    
    int i;
    for(i = 0; i < mem_index; i++) {
        printf("stored memory - (%d, %s, %s)\n", memTable[i].index, &memTable[i].key, &memTable[i].value);
    }
    if(mem_index >= MAX_MEM_SIZE) {
        return 1;
    }
    return 0;
}

int write_to_file() {
    printf("wirte to file\n");
    int i;
    int temp_file_number = get_max_filename(".st") + 1; 
    char filename[32];
    printf("temp_file_number = %d\n", temp_file_number);

    sprintf(filename,"./%d.st", temp_file_number);
    printf("create file: %s\n", filename);
    FILE *fp = fopen(filename, "w+");
    if(fp == NULL) {
        printf("File Open Error\n");
        return 0;
    }

    for(i=0; i < mem_index; i++) {
        fprintf(fp, "%d %s %s\n", memTable[i].index, &memTable[i].key, &memTable[i].value);
        printf("stored in %s (%d, %s, %s)\n",filename, memTable[i].index, &memTable[i].key, &memTable[i].value);
    }
    mem_index = 0;
    memset(memTable, 0, sizeof(memTable));
    fclose(fp);
    get_file_count();
    return 0;
}

int flush(keyValue data){
    printf("push\n");
    data.key[4] = '\0';
    if(write_to_memory(data) >= 1) {
        write_to_file();
    }

}

int find_memory_on(keyValue* result, char* search_key) {
    int i = 0;
    for(i = mem_index-1; i >= 0; i--) {
        if(!strcmp(memTable[i].key, search_key)) {
            result->index = memTable[i].index;
            strcpy(result->key, memTable[i].key);
            strcpy(result->value, memTable[i].value);
            return 1;
        }
    }
    return 0;
}

int find_file(keyValue* result, unsigned char* search_key, char* fileName) {
    printf("find file\n");
    FILE* fp = fopen(fileName, "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 0;
    }

    unsigned char line[MAX_BUFF];
    char* token;
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("line : %s", &line);
        // 개행 문자 제거
        line[strcspn(line, "\n")] = 0;

        // 문자열을 리하여 구조체 멤버에 저장
        token = strtok(line, " ");
        result->index = atoi(token);

        token = strtok(NULL, " ");
        strcpy(result->key, token);

        int i;
        token = strtok(NULL, " ");
        for (i = 0; i < strlen(token); i++) {
            result->value[i] = token[i];
        }
        result->value[strlen(token)] = '\0';

        if(strcmp(result->key,search_key) == 0) {
            return 1;
        }
    }
    return 0;
}

int find_on_disk(keyValue* result, unsigned char* search_key) {
    int temp_file_number = get_max_filename(".st"); 
    char filename[32];
    int i = 0;
    int flag = *ipc_find_order_addr;
    if(flag == 0) {
        for(i = 0; i < 3; i++) {
            sprintf(filename,"./%d.st", temp_file_number -i);
            printf("filename : %s\n", &filename);
            if(find_file(result, search_key, filename)){
                return 1;
            }
        }
    }
    else {
        sprintf(filename,"./%d.st", temp_file_number - 1);
        printf("filename : %s\n", &filename);
        if(find_file(result, search_key, filename)){
            return 1;
        }

        sprintf(filename,"./%d.st", temp_file_number);
        printf("filename : %s\n", &filename);
        if(find_file(result, search_key, filename)){
            return 1;
        }
    }
    return 0;
}

/**
 * Device Control
*/
// LED device control function using mmap
int led_mmap(unsigned char data){
    int fd,i;

	unsigned long *fpga_addr = 0;
	unsigned char *led_addr =0;

	if( (data<0) || (data>255) ){
		printf("Invalid range!\n");
		return 0;
	}

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("/dev/mem open error\n");
		return 0;
	}

	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPGA_BASE_ADDRESS);
	if (fpga_addr == MAP_FAILED)
	{
		printf("mmap error!\n");
		close(fd);
		return 0;
	}
	
	led_addr=(unsigned char*)((void*)fpga_addr+LED_ADDR);
	*led_addr=data; //write led
	
	munmap(led_addr, 4096);
	close(fd);

    return 1;
}

// LED control functions
void led_reset(){
    led_mmap((unsigned char)0);
}

void led_init(){
    led_mmap((unsigned char)128);
}

void led_finished_put(){
    led_mmap((unsigned char)255);
}

void led_blink_key(){
    led_mmap((unsigned char)32);
    sleep(1);
    led_mmap((unsigned char)16);
}

void led_blink_value(){
    led_mmap((unsigned char)2);
    sleep(1);
    led_mmap((unsigned char)1);
}

// run in ohter thread
void* led_controller(){
    while(exit_flag == 0) {
        if(led_flag == 1) {
            led_init();
        }
        else if(led_flag == 34){
            led_blink_key();
        }
        else if(led_flag == 78){
            led_blink_value();
        }
        else {
            led_finished_put();
            led_flag = 1;
        }
        sleep(1);
    }
    led_reset();
}

/**
 * reset key controller
 */ 
void* reset_key_controller(){
    int dev;
	unsigned char dip_sw_buff = 0;  

	dev = open("/dev/fpga_dip_switch", O_RDWR);

	if (dev<0){
		printf("Device Open Error\n");
		close(dev);
		return;
	}
    
	while(exit_flag == 0){
		usleep(300000);
		read(dev, &dip_sw_buff, 1);
        if(dip_sw_buff == 0){
            printf("Read dip switch: 0x%02X \n", dip_sw_buff);
            keyValue_flag++;
            keyValue_flag = keyValue_flag%2;
            if(keyValue_flag == 1) {
                led_flag = 78;
            }
            if(keyValue_flag == 0) {
                led_flag = 34;
            }
        }
	}
}


/**
 * Ready Key controller using device driver
 */
void* readyKey_contorller(){
    struct input_event ev[BUFF_SIZE];
	int fd, rd, value, size = sizeof (struct input_event);

	char* device = "/dev/input/event0";
	if((fd = open (device, O_RDONLY)) == -1) {
		printf ("%s is not a vaild device.n", device);
	}
    printf("[device open] readykey\n");

	while (exit_flag == 0){
		if ((rd = read (fd, ev, size * BUFF_SIZE)) < size)
		{
			printf("read()");  
			return;     
		}

        value = ev[0].value;
        if(value && ev[0].code == 158){
            *ipc_exit_addr = 1;
            return;
        }

        else if(value && ev[0].code == 115) {
            input_flag++;
            input_flag = input_flag%3;
            led_flag = 1;
        }

        else if(value && ev[0].code == 114) {
            if(input_flag == 0) {
                input_flag = 2;
            }
            else {
                input_flag--;
            }
            led_flag = 1;
        }
        if(input_flag % 3 == 2) {
            *ipc_merge_addr = 3;
        }
	}

	return;
}


/**
 * Motor
 */
 void* motor_controller() {
	int dev;
    int motor_flag = 0;
	int motor_action = 1;
	int motor_direction = 1;
	int motor_speed = 2;
	unsigned char motor_state[3];
	
	memset(motor_state,0,sizeof(motor_state));
	dev = open(FPGA_STEP_MOTOR_DEVICE, O_WRONLY);
	if (dev < 0) {
		printf("Device open error : %s\n",FPGA_STEP_MOTOR_DEVICE);
		return;
	}
    printf("[device open] motor\n");


    while(exit_flag == 0) {
        motor_flag = *ipc_motor_flag_addr;
        if(motor_flag == 1) {
            printf("run motor \n");
            motor_action = 1;
            motor_state[0]=(unsigned char)motor_action;
            motor_state[1]=(unsigned char)motor_direction;;
            motor_state[2]=(unsigned char)motor_speed;	
            write(dev,motor_state,3);	
            sleep(2);
            *ipc_motor_flag_addr = 0;
        }

        motor_action = 0;
        motor_state[0]=(unsigned char)motor_action;
        motor_state[1]=(unsigned char)motor_direction;;
        motor_state[2]=(unsigned char)motor_speed;
        write(dev,motor_state,3);	
    }
	close(dev);
	
	return;
}


/**
 * FND
*/
void* fnd_contorller() {
	int dev, i;
	unsigned char retval;
    unsigned char copy_data[4];

    dev = open(FND_DEVICE, O_RDWR);
    if (dev<0) {
        printf("Device open error : %s\n",FND_DEVICE);
        exit(1);
    }

    for(i = 0; i < 4; i++) {
        fnd_data[i] = '0'-0x30;
    }
    
    while(exit_flag == 0) {
        memset(copy_data,0,sizeof(copy_data));
        for(i = 0; i < 4; i++) {
            copy_data[i] = fnd_data[i];
        }
   
        retval=write(dev,&copy_data, 4);	
        if(retval<0) {
            printf("Write Error!\n");
            return;
        }
    }

    memset(copy_data,0,sizeof(copy_data));
    retval=write(dev,&copy_data, 4);	

    close(dev);
    return;
}

/**
 * Text LCD
*/

void text_lcd_controller(unsigned char text_lcd_data[MAX_BUFF]) {
	int dev;		
	dev = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);
	if (dev<0) {
		printf("Device open error : %s\n",FPGA_TEXT_LCD_DEVICE);
		return;
	}
    int i = 0;
    for(i=0; i<MAX_BUFF; i++){
        if(text_lcd_data[i] == '\0') {
            text_lcd_data[i] = ' ';
        }
    }
    printf("[device open] - text lcd\n");
	write(dev,text_lcd_data,MAX_BUFF);	
	
	close(dev);
}

/**
 * Switch
*/

unsigned char change_num_to_eng(unsigned char temp){
    if(temp == '1') {
        return '.';
    }
    if(temp == '2') {
        return 'A';
    }
    if(temp == '3') {
        return 'D';
    }
    if(temp == '4') {
        return 'G';
    }
    if(temp == '5') {
        return 'J';
    }
    if(temp == '6') {
        return 'M';
    }
    if(temp == '7') {
        return'P';
    }
    if(temp == '8') {
        return 'T';
    }
    if(temp == '9') {
        return 'W';
    }
    return 0;
}

unsigned char change_eng_to_num(unsigned char temp){
    if(temp == '.' || temp == 'Q') {
        return '1';
    }
    if(temp == 'A' || temp == 'B') {
        return '2';
    }
    if(temp == 'D' || temp == 'E') {
        return '3';
    }
    if(temp == 'G' || temp == 'H') {
        return '4';
    }
    if(temp == 'J' || temp == 'K') {
        return '5';
    }
    if(temp == 'M' || temp == 'N') {
        return '6';
    }
    if(temp == 'P' || temp == 'R') {
        return '7';
    }
    if(temp == 'T' || temp == 'U') {
        return '8';
    }
    if(temp == 'W' || temp == 'X') {
        return '9';
    }
    return 0;
}

unsigned char change_eng_to_eng(unsigned char temp){
    if(temp == 'A') {
        return 'B';
    }
    if(temp == 'B') {
        return 'C';
    }
    if(temp == 'D') {
        return 'E';
    }
    if(temp == 'E') {
        return 'F';
    }
    if(temp == 'G') {
        return 'H';
    }
    if(temp == 'H') {
        return 'I';
    }
    if(temp == 'J') {
        return 'K';
    }
    if(temp == 'K') {
        return'L';
    }
    if(temp == 'P') {
        return'R';
    }
    if(temp == 'R') {
        return 'S';
    }
    if(temp == '.') {
        return 'Q';
    }
    if(temp == 'Q') {
        return'Z';
    }
    if(temp == 'P') {
        return'R';
    }
    if(temp == 'R') {
        return 'S';
    }
    if(temp == 'T') {
        return'U';
    }
    if(temp == 'U') {
        return 'V';
    }
    if(temp == 'W') {
        return'X';
    }
    if(temp == 'X') {
        return 'Y';
    }
    return 0;
}
void* switch_controller() {
	int fnd_index = 0;
    int led_index = 0;

    keyValue_flag = 0;
    int num_eng_flag = 0;
	int buff_size;
    int i;
    unsigned char input_led_data[MAX_BUFF];
	unsigned char push_sw_buff[MAX_BUTTON];

    // init
    for(i = 0; i < buff_size; i++) {
        fnd_data[i] = '0' - 0x30;
    }

	int dev = open("/dev/fpga_push_switch", O_RDWR);
	if (dev<0){
		printf("Device Open Error\n");
		close(dev);
		return;
	}

    printf("[device open] switch\n");
	buff_size=sizeof(push_sw_buff);

    // init text lcd
    memset(input_led_data,' ',MAX_BUFF);
    text_lcd_controller(input_led_data);

    int temp_input_flag = 0;
    while(exit_flag == 0){
        usleep(200000);
		read(dev, &push_sw_buff, buff_size);
        int count = 0;
        unsigned char temp = '0';
		for(i=0;i<MAX_BUTTON;i++) {
			if(push_sw_buff[i]) {
                temp += i + 1;
                count++;
            }
		}
        if(input_flag != temp_input_flag) {
            memset(input_led_data,' ',MAX_BUFF);
            text_lcd_controller(input_led_data);
            fnd_index = 0;
            for(i = 0; i < 4; i++) {
                fnd_data[i] = '0' - 0x30;
            }
            temp_input_flag = input_flag;
        }

        // Put Mode
        if(input_flag == 0) {
            if(count == 1) {
                // input key
                if(keyValue_flag == 0) {
                    led_flag = 34;
                    if(num_eng_flag != 0) {
                        printf("[Warning] Key must be number\n");
                    }
                    if(fnd_index <= 3) {
                        
                        printf("add number on FND\n");
                        fnd_data[fnd_index] = temp - 0x30;
                        fnd_index++;
                    }
                    else {
                        printf("fnd is full. please input vlaue\n");
                    }
                }
                // input value
                else if(keyValue_flag == 1) {
                    if(num_eng_flag == 0) {
                        printf("add number on text lcd\n");
                        input_led_data[led_index] = temp;
                        led_index++;
                        memset(input_led_data+led_index,' ',MAX_BUFF-led_index);
                        text_lcd_controller(input_led_data);
                    }
                    else {
                        printf("add alphabet on text lcd\n");
                        if(change_eng_to_num(input_led_data[led_index-1]) == temp) {
                            input_led_data[led_index-1] = change_eng_to_eng(input_led_data[led_index-1]);
                        }
                        else {
                            input_led_data[led_index] = change_num_to_eng(temp);
                            led_index++;
                        }

                        memset(input_led_data+led_index,' ',MAX_BUFF-led_index);
                        text_lcd_controller(input_led_data);
                    }
                }
            }
            if(count == 2) {
                // reset
                if(push_sw_buff[1] && push_sw_buff[2]) {
                    // key initialization (FND)
                    if(keyValue_flag == 0) {
                        printf("reset key on FND\n");
                        fnd_index = 0;
                        for(i = 0; i < 4; i++) {
                            fnd_data[i] = '0' - 0x30;
                        }
                    } 
                    // value text initialization (text lcd)
                    else if(keyValue_flag == 1) {
                        printf("reset value on text lcd\n");
                        memset(input_led_data,' ',MAX_BUFF);
                        text_lcd_controller(input_led_data);
                        led_index = 0;
                    }
                }

                // change input character
                if(push_sw_buff[4] && push_sw_buff[5]) {
                    printf("num to eng / eng to num\n");
                    num_eng_flag++;
                    num_eng_flag = num_eng_flag%2;
                }

                // push !!!!!!!!! 6 to 7 
                if(push_sw_buff[6] && push_sw_buff[8]) {
                    printf("push to main process \n");
                    for(i = 0; i < 4; i++) {
                        put_key_addr[i] = fnd_data[i] + 0x30;
                    }
                    put_key_addr[4] = '\0';
                    for(i = 0; i < MAX_BUFF; i++) {
                        if(input_led_data[i]==' ') {
                            input_led_data[i] = '\0';
                        }
                    }
                    strcpy(put_value_addr, input_led_data);
                    *ipc_mode_flag_addr = 1;

                    fnd_index = 0;
                    led_index = 0;
                    keyValue_flag = 0;
                    led_flag = 0;

                    for(i = 0; i < 4; i++) {
                        fnd_data[i] = '0'-0x30;
                    }

                    memset(input_led_data,' ',MAX_BUFF);
                    text_lcd_controller(input_led_data);
                }
            }
        }
        // Get Mode
        else if(input_flag == 1) {
            if(count == 1) {
                // input key
                led_flag = 34;
            }
            if(count == 1) {
                if(fnd_index <= 3) {
                    printf("add number on FND\n");
                    fnd_data[fnd_index] = temp - 0x30;
                    fnd_index++;
                }
            }
            if(count == 2) {
                if(push_sw_buff[6] && push_sw_buff[8]) {
                    // Get KEY
                    for(i = 0; i < 4; i++) {
                        get_key_addr[i] = fnd_data[i] + 0x30;
                    }
                    get_key_addr[4] = '\0';
                    *ipc_mode_flag_addr = 2;
                    printf("reset key on FND\n");
                    fnd_index = 0;
                    for(i = 0; i < 4; i++) {
                        fnd_data[i] = '0' - 0x30;
                    }
                }
                if(push_sw_buff[1] && push_sw_buff[2]) {
                    // key initialization (FND)
                    printf("reset key on FND\n");
                    fnd_index = 0;
                    for(i = 0; i < 4; i++) {
                        fnd_data[i] = '0' - 0x30;
                    }
                }
            }

        }
        else {
            printf("merge mode now. \n chage mode\n");
        }
	}
	close(dev);
}


/**
 * init
*/

int get_data_index() {
    char line[32];
    FILE* fp = fopen("index", "r");
    if (fp == NULL) {
        return 0;
    }    
    fgets(line, 32, fp);
    data_index = atoi(line);
    printf("data index : %d\n", data_index);
    fclose(fp);
    return data_index;
}

int write_data_index(){
    FILE* fp = fopen("index", "w");
    if (fp == NULL) {
        return 1;
    }    
    fprintf(fp,"%d", data_index);
    printf("data index : %d\n", data_index);
    fclose(fp);
}

/**
 * Process
 */
// Main process
int mainProcess() {
    mem_index = 0;
    data_index = get_data_index();
    printf("data_index222 : %d\n", data_index);
    volatile int mode_flag;
    memset(memTable, 0, sizeof(memTable));

    while(exit_flag == 0) {
        exit_flag = *ipc_exit_addr;
        // default
        mode_flag = *ipc_mode_flag_addr;
        if(mode_flag == 0) {
        }
        // put mode
        if(mode_flag == 1) {
            printf("put mode!!\n");
            keyValue temp_data;
            data_index++;
            printf("data_index333 : %d\n", data_index);
            temp_data.index = data_index; 
            strcpy(temp_data.key, put_key_addr);
            strcpy(temp_data.value, put_value_addr);
            flush(temp_data);
            printf("push to memory in main porcess\n");
            *ipc_mode_flag_addr = 0;
            mode_flag = 0;
        } 
        // get mode
        if(mode_flag == 2) {
            keyValue temp_data; 
            unsigned char result[MAX_BUFF];
            unsigned char search_key[5];    
            strcpy(search_key, get_key_addr);
            if(find_memory_on(&temp_data, search_key))  {
                get_key_value_addr->index = temp_data.index;
                strcpy(get_key_value_addr->key, temp_data.key);
                strcpy(get_key_value_addr->value,temp_data.value);
                *ipc_read_get_key_value_flag_addr = 1;
            }
            else if (find_on_disk(&temp_data, search_key)) {
                get_key_value_addr->index = temp_data.index;
                strcpy(get_key_value_addr->key, temp_data.key);
                strcpy(get_key_value_addr->value,temp_data.value);
                *ipc_read_get_key_value_flag_addr = 1;
            }
            else {
                get_key_value_addr->index = 0;
                strcpy(get_key_value_addr->key, "ERROR");
                strcpy(get_key_value_addr->value, "NO_DATA");
                *ipc_read_get_key_value_flag_addr = 1;
            }
            *ipc_mode_flag_addr = 0;
            mode_flag = 0;
        }
    }

    if(mem_index != 0) {
        write_to_file();
    }

    write_data_index();
    
    sleep(3);
    *ipc_exit_merge_addr = 1;
    return 1;
}

// I/O process
int ioProcess() {
    input_flag = 0; // input mode Flag
    printf("i/o process\n");

    // LED Control Thread
    pthread_t led_thread;
    led_flag = 0;  // led control variable
    pthread_create(&led_thread, NULL, led_controller, NULL);

    // readyKey Control Thread
    pthread_t readyKey_thread;
    pthread_create(&readyKey_thread, NULL, readyKey_contorller, NULL);

    // Motor Control Thread
    pthread_t motor_thread;
    pthread_create(&motor_thread, NULL, motor_controller, NULL);

    // FND Control Thread
    pthread_t fnd_thread;
    pthread_create(&fnd_thread, NULL, fnd_contorller, NULL);

    // Switch Control Thread
    pthread_t switch_thread;
    pthread_create(&switch_thread, NULL, switch_controller, NULL);

    // reset Control Thread
    pthread_t reset_thread;
    pthread_create(&reset_thread, NULL, reset_key_controller, NULL);
    printf("finish create threads\n");


    // key value state 
    int read_key_value_flag = 0;
    int i = 0;
    unsigned char text_data[MAX_BUFF];
    while(exit_flag == 0) {
        exit_flag = *ipc_exit_addr;
        read_key_value_flag = *ipc_read_get_key_value_flag_addr;
        if(read_key_value_flag == 1) {
            keyValue find_key;
            find_key.index = get_key_value_addr->index;
            strcpy(find_key.key, get_key_value_addr->key);
            strcpy(find_key.value, get_key_value_addr->value);
            memset(text_data,' ', MAX_BUFF);
            sprintf(text_data, "(%d, %s, %s) \0", find_key.index, &find_key.key ,&find_key.value);
            printf("(%d, %s, %s)\0", find_key.index, &find_key.key, &find_key.value);
            text_lcd_controller(text_data);
            *ipc_read_get_key_value_flag_addr = 0;
            read_key_value_flag = 0;
        }
    }

    //reset 
    led_reset();
    *ipc_motor_flag_addr = 0;
    memset(text_data,' ', MAX_BUFF);
    text_lcd_controller(text_data);
    for(i = 0; i < 4; i++) {
        fnd_data[i] = '0'- 0x30;
    }
    sleep(2);
    return 1;
}

// Merge Process
int mergeProcess() {
    printf("merge process\n");
    volatile int merge_flag = 0;
    back_ground_flag = 0;
    while(exit_flag == 0) {
        exit_flag = *ipc_exit_merge_addr;
        merge_flag = *ipc_merge_addr;
        if(merge_flag >= 3) {
            if(get_file_count() >= 3) {
                back_ground_flag = 1;
            }
            *ipc_motor_flag_addr = 1;
            printf("run merge to mergeMode\n");
            merge();
            get_file_count();
        }

    }
    return 1;
}

int main() {
    exit_flag = 0;
    input_flag = 0;

    // put mode에서 입력받은 key
    put_key = shmget(IPC_PRIVATE, SHM_SIZE_KEY, IPC_CREAT|SHM_CREATE); 
    // put mode에서 입력받은 value
    put_value = shmget(IPC_PRIVATE, SHM_SIZE_VALUE, IPC_CREAT|SHM_CREATE);
    // get mode에서 입력받은 key
    get_key = shmget(IPC_PRIVATE, SHM_SIZE_KEY, IPC_CREAT|SHM_CREATE);
    // get mode에서 입력받은 key를 기반으로 찾은 keyValue data
    get_key_value = shmget(IPC_PRIVATE, SHM_SIZE_KEY_VALUE, IPC_CREAT|SHM_CREATE);
    // memTable을 shared memory로
    memTable_pointer = shmget(IPC_PRIVATE, SHM_SIZE_KEY_VALUE*3, IPC_CREAT|SHM_CREATE);

    // flag
    ipc_mode_flag = shmget(IPC_PRIVATE, SHM_SIZE_INTEGER, IPC_CREAT|SHM_CREATE);
    ipc_motor_flag = shmget(IPC_PRIVATE, SHM_SIZE_INTEGER, IPC_CREAT|SHM_CREATE);
    ipc_merge = shmget(IPC_PRIVATE, SHM_SIZE_INTEGER, IPC_CREAT|SHM_CREATE);
    ipc_exit = shmget(IPC_PRIVATE, SHM_SIZE_INTEGER, IPC_CREAT|SHM_CREATE);
    ipc_read_get_key_value_flag = shmget(IPC_PRIVATE, SHM_SIZE_INTEGER, IPC_CREAT|SHM_CREATE);
    ipc_exit_merge = shmget(IPC_PRIVATE, SHM_SIZE_INTEGER, IPC_CREAT|SHM_CREATE);
    ipc_find_order = shmget(IPC_PRIVATE, SHM_SIZE_INTEGER, IPC_CREAT|SHM_CREATE);

    memTable = shmat(memTable_pointer, NULL, 0);
    if(put_key_addr == (void*)-1) {
        perror("[Error] shmat failed - memTalbe\n");
    }
    
    put_key_addr = shmat(put_key, NULL, 0);
    if(put_key_addr == (void*)-1) {
        perror("[Error] shmat failed - put_key_addr\n");
    }

    put_value_addr = shmat(put_value, NULL, 0);
    if(put_value_addr == (void*)-1) {
        perror("[Error] shmat failed - put_value_addr\n");
    }

    get_key_addr = shmat(get_key, NULL, 0);
    if(get_key_addr == (void*)-1) {
        perror("[Error] shmat failed - get_key_addr\n");
    }

    get_key_value_addr = shmat(get_key_value, NULL, 0);
    if(get_key_value_addr == (void*)-1) {
        perror("[Error] shmat failed - get_key_value_addr\n");
    }

    ipc_mode_flag_addr = shmat(ipc_mode_flag, NULL, 0);
    if(ipc_mode_flag_addr == (void*)-1) {
        perror("[Error] shmat failed - ipc_mode_flag_addr\n");
    }

    ipc_motor_flag_addr = shmat(ipc_motor_flag, NULL, 0);
    if(ipc_motor_flag_addr == (void*)-1) {
        perror("[Error] shmat failed - ipc_motor_flag_addr\n");
    }

    ipc_merge_addr = shmat(ipc_merge, NULL, 0);
    if(ipc_merge_addr == (void*)-1) {
        perror("[Error] shmat failed - ipc_merge_addr\n");
    }

    ipc_exit_addr = shmat(ipc_exit, NULL, 0);
    if(ipc_exit_addr== (void*)-1) {
        perror("[Error] shmat failed - ipc_exit_addr\n");
    }

    ipc_read_get_key_value_flag_addr = shmat(ipc_read_get_key_value_flag, NULL, 0);
    if(ipc_read_get_key_value_flag_addr== (void*)-1) {
        perror("[Error] shmat failed - ipc_read_get_key_value_flag_addr\n");
    }

    ipc_exit_merge_addr = shmat(ipc_exit_merge, NULL, 0);
    if(ipc_exit_merge_addr== (void*)-1) {
        perror("[Error] shmat failed - ipc_exit_merge_addr\n");
    }

    ipc_find_order_addr = shmat(ipc_find_order, NULL, 0);
    if(ipc_find_order_addr== (void*)-1) {
        perror("[Error] shmat failed - ipc_find_order_addr\n");
    }
    *ipc_mode_flag_addr = 0;
    *ipc_merge_addr = 0;
    *ipc_exit_addr = 0;
    *ipc_exit_merge_addr = 0;
    if(get_file_count() == 1) {
        *ipc_find_order_addr = 1;
    } else {
        *ipc_find_order_addr = 0;
    }

    int pid = 0;
    pid = fork();
    if(pid == 0) {
        if(!ioProcess()) {
            printf("[Error] From I/O Process\n");
        }
    } 
    
    else if(pid > 0) {
        pid = 0;
        pid = fork();
        
        if(pid == 0) {
            if(!mergeProcess()){
                printf("[Error] From Merge Process\n");
            }
        } 
        
        else if(pid > 0) {
            if(!mainProcess()) {
                printf("[Error] From Main Process\n");
            }
        }
    } 
    
    else {
        printf("[Error] Fork Miss\n");
        return -1;
    }

    shmdt(put_key_addr);
    shmdt(put_value_addr);
    shmdt(get_key_addr);
    shmdt(get_key_value_addr);
    shmdt(ipc_mode_flag_addr);
    shmdt(ipc_exit_addr);
    shmdt(ipc_read_get_key_value_flag_addr);
    shmdt(ipc_exit_merge_addr);
    shmdt(ipc_find_order_addr);

    shmctl(put_key, IPC_RMID, 0);
    shmctl(put_value, IPC_RMID, 0);
    shmctl(get_key, IPC_RMID, 0);
    shmctl(get_key_value, IPC_RMID, 0);
    shmctl(ipc_mode_flag, IPC_RMID, 0);
    shmctl(ipc_exit, IPC_RMID, 0);
    shmctl(ipc_read_get_key_value_flag, IPC_RMID, 0);
    shmctl(ipc_exit_merge, IPC_RMID, 0);
    shmctl(ipc_find_order, IPC_RMID, 0);
    printf("program exit\n");
    return 1;
}
