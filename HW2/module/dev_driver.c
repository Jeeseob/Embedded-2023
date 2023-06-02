/* DEV Ioremap Control
FILE : dev_driver.c */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/delay.h>
#include "ioctl.h"


#define IOM_DEV_MAJOR 242		// ioboard dev device major number
#define IOM_DEV_NAME "dev_driver"		// ioboard dev device name
#define ledAddr 0x16
#define fndAddr 0x4
#define text_lcdAddr 0x90
#define dotAddr 0x210
#define IOM_DEV_ADDRESS 0x08000000 // pysical address
#define MAX_BUFF 32
#define LINE_BUFF 16


//Global variable
static int devport_usage = 0;
static unsigned char *iom_fpga_dev_addr;
char student_number[LINE_BUFF] = "120230200       ";
char name[LINE_BUFF] = "kimjeeseob      ";
struct ioctl_info value;
int st_flag = 1;
int name_flag = 1;

// define functions...
static long dev_ioctl(struct file *file, unsigned int command, unsigned long arg);
int iom_dev_open(struct inode *minode, struct file *mfile);
int iom_dev_release(struct inode *minode, struct file *mfile);

// define file_operations structure 
struct file_operations iom_dev_fops =
{
	.owner		=	THIS_MODULE,
	.open		=	iom_dev_open,
	.unlocked_ioctl      =   dev_ioctl,
	.release	=	iom_dev_release,
};

unsigned char find_setting_number(unsigned char *timer_char){
    int i;
    for(i=0; i<4; i++) {
        if(timer_char[i] != 0) {
            break;
        }
    }

    printk(KERN_INFO "i = %d, char = %d\n", i, timer_char[i]);
    return timer_char[i];
}

int find_setting_fnd(unsigned char *timer_char){
    int i;
    for(i=0; i<4; i++) {
        if(timer_char[i] != 0) {
            return i;
        }
    }

    return -1;
}

void write_led(unsigned char value) {
    unsigned short _s_value;
    _s_value = (unsigned short)value;
    outw(_s_value, (unsigned int)iom_fpga_dev_addr + ledAddr);
}

// when write to led device  ,call this function
void set_led(unsigned char *timer_char) 
{
    unsigned char value;
    value = ledValue[find_setting_number(timer_char)];
    write_led(value);
    printk(KERN_INFO "led Set %d\n", value);
}

unsigned char get_led(void)
{
    unsigned char value;
	unsigned short _s_value;

	_s_value = inw((unsigned int)iom_fpga_dev_addr + ledAddr);	    
    value = _s_value;
    printk(KERN_INFO "led Get %d\n",value);
	return value;
}

void write_dot(int temp_value) 
{
    int i;
	unsigned short int _s_value;
	if(temp_value == -1) {
        for(i=0;i<10;i++){
            _s_value = fpga_set_blank[i];
            outw(_s_value,(unsigned int)iom_fpga_dev_addr+dotAddr+i*2);
        }
    }
    else {
        for(i=0;i<10;i++)
        {
            _s_value = fpga_number[temp_value][i];
            outw(_s_value,(unsigned int)iom_fpga_dev_addr+dotAddr+i*2);
        }
    }
}

void set_dot(unsigned char *timer_char) 
{
    int temp_value;
    unsigned char temp;
    temp = find_setting_number(timer_char);
    temp_value = (int)temp;
    printk(KERN_INFO "temp_value : %d,", temp_value);
    write_dot(temp_value);
}

void set_fnd(unsigned char *timer_char) 
{
	unsigned short int value_short = 0;
    value_short = timer_char[0] << 12 | timer_char[1] << 8 |timer_char[2] << 4 |timer_char[3];
    outw(value_short,(unsigned int)iom_fpga_dev_addr + fndAddr);	    
}

void get_fnd(unsigned char* value)
{
	unsigned short int value_short = 0;

    value_short = inw((unsigned int)iom_fpga_dev_addr + fndAddr);	    
    value[0] =(value_short >> 12) & 0xF;
    value[1] =(value_short >> 8) & 0xF;
    value[2] =(value_short >> 4) & 0xF;
    value[3] = value_short & 0xF;
}

void set_text_lcd(void)
{
	int i;
    unsigned short int _s_value = 0;
	unsigned char value[MAX_BUFF + 1];

	memset(value,0,sizeof(value));	
	strncat(value, student_number, LINE_BUFF);
	strncat(value, name, LINE_BUFF);

	value[MAX_BUFF]=0;
	printk("Get Size : %d \n String : %s\n",MAX_BUFF, value);

	for(i=0; i<MAX_BUFF; i++)
    {
        _s_value = ((value[i] & 0xFF) << 8) | value[i + 1] & 0xFF;
		outw(_s_value,(unsigned int)iom_fpga_dev_addr+text_lcdAddr+i);
        i++;
    }
}

void move_text_lcd(void)
{
	int i;
    unsigned short int _s_value = 0;
	unsigned char value[MAX_BUFF + 1];
    // shift flag
    if(student_number[0] != ' ') {
        st_flag = 1;
    } else if(student_number[LINE_BUFF-1] != ' ') {
        st_flag = 0;
    }

    if(name[0] != ' ') {
        name_flag = 1;
    } else if(name[LINE_BUFF-1] != ' ') {
        name_flag = 0;
    }

    // shift
    if(st_flag == 1) {
        for(i=LINE_BUFF-2; i>=0; i--) {
            student_number[i+1] = student_number[i];
        }
        student_number[0] = ' ';
    } else {
        for(i=0; i<LINE_BUFF-1; i++) {
            student_number[i] = student_number[i+1];
        }
        student_number[LINE_BUFF-1] = ' ';
    }
    if(name_flag == 1) {
        for(i=LINE_BUFF-2; i>=0; i--) {
            name[i+1] = name[i];
        }
        name[0] = ' ';
    } else {
        for(i=0; i<LINE_BUFF-1; i++) {
            name[i] = name[i+1];
        }
        name[LINE_BUFF-1] = ' ';
    }

	memset(value,0,sizeof(value));	
	strncat(value, student_number, LINE_BUFF);
	strncat(value, name, LINE_BUFF);

	value[MAX_BUFF]=0;
	printk("Get Size : %d \n String : %s\n",MAX_BUFF, value);

	for(i=0; i<MAX_BUFF; i++)
    {
        _s_value = ((value[i] & 0xFF) << 8) | value[i + 1] & 0xFF;
		outw(_s_value,(unsigned int)iom_fpga_dev_addr+text_lcdAddr+i);
        i++;
    }
}

void init_text_lcd(void)
{
	int i;
    unsigned short int _s_value = 0;
	unsigned char value[MAX_BUFF + 1];

	memset(value,' ',MAX_BUFF);
	for(i=0; i<=MAX_BUFF; i++)
    {
        _s_value = ((value[i] & 0xFF) << 8) | value[i + 1] & 0xFF;
		outw(_s_value,(unsigned int)iom_fpga_dev_addr+text_lcdAddr+i);
        i++;
    }
}


void do_timer(struct ioctl_info* value) 
{
    int i;
    int count;
    int fnd_data;
    unsigned char led_temp;
    unsigned char temp;
    unsigned char fnd_temp[4];
    unsigned char fnd_value[4];
    count = value->timer_cnt;
    while(count > 1) {
        // update led & dot
        led_temp = get_led();
        printk(KERN_INFO "led_temp: %d\n", led_temp);
        if(led_temp == ledValue[8]) {
            write_led(ledValue[1]);
            write_dot(1);
        }
        else {
            for(i=1; i < 8; i++) {
                if(ledValue[i] == led_temp) {
                    write_led(ledValue[++i]);
                    write_dot(i);
                    break;
                }
            }
        }

        //update fnd
        get_fnd(fnd_temp);
        fnd_data = find_setting_fnd(fnd_temp);
        for(i=0;i<4;i++){
            fnd_value[i] = 0;
        }
        if(fnd_temp[fnd_data] == 8) {
            fnd_value[fnd_data] = 1;
        } 
        else {
            fnd_value[fnd_data] = fnd_temp[fnd_data]+1;
        } 

        if(fnd_value[fnd_data] == value->timer_char[find_setting_fnd(value->timer_char)]) {
            temp = fnd_value[3];
            for(i=2; i>=0; i--) {
                fnd_value[i+1] = fnd_value[i];
            }
            fnd_value[0] = temp;
        }
        set_fnd(fnd_value);

        // update text lcd
        move_text_lcd();

        // print log
        printk(KERN_INFO "timer count: %d\n", count);
        
        // sleep time interval
        msleep(value->timer_interval * 100);
        count--;
    }
    // initialize
    write_led(ledValue[0]);
    write_dot(-1);
    init_text_lcd();

    for(i = 0; i<4; i++) {
        fnd_value[i] = 0;
    }
    set_fnd(fnd_value);
    strcpy(student_number, "120230200       ");
    strcpy(name, "kimjeeseob      ");
}
static long dev_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
    switch (command)
    {
    case SET_OPTIONS:
        if(copy_from_user(&value, (struct ioctl_info __user *)arg, sizeof(struct ioctl_info))) {
            return -EFAULT;
        }
        printk(KERN_INFO "Received SET_OPTIONS value1: %d, value2: %d\n", value.timer_interval, value.timer_cnt);
        set_led(value.timer_char);
        set_fnd(value.timer_char);
        set_dot(value.timer_char);
        set_text_lcd();
        break;
    
    case START_TIMER:
        printk(KERN_INFO "START TIMER\n");
        printk(KERN_INFO "Received START_TIMER value1: %d, value2: %d\n", value.timer_interval, value.timer_cnt);
        do_timer(&value);
        printk(KERN_INFO "END TIMER\n");
        break;

    default:
        printk("Command is not defined!\n");
        break;
    }
    return 1;
}

// when dev device open ,call this function
int iom_dev_open(struct inode *minode, struct file *mfile) 
{	
	if(devport_usage != 0) return -EBUSY;

	devport_usage = 1;

	return 0;
}

// when dev device close ,call this function
int iom_dev_release(struct inode *minode, struct file *mfile) 
{
	devport_usage = 0;

	return 0;
}

int __init iom_dev_init(void)
{
	int result;

	result = register_chrdev(IOM_DEV_MAJOR, IOM_DEV_NAME, &iom_dev_fops);
	if(result < 0) {
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}

	iom_fpga_dev_addr = ioremap(IOM_DEV_ADDRESS, 0x300);

	printk("init module, %s major number : %d\n", IOM_DEV_NAME, IOM_DEV_MAJOR);

	return 0;
}

void __exit iom_dev_exit(void) 
{
	iounmap(iom_fpga_dev_addr);
	unregister_chrdev(IOM_DEV_MAJOR, IOM_DEV_NAME);
}

module_init(iom_dev_init);
module_exit(iom_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeeseob");
