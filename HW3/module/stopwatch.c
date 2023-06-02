#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/time.h>

#define IOM_FND_MAJOR 242		// ioboard fpga device major number
#define IOM_FND_NAME "stopwatch"		// ioboard fpga device name
#define IOM_FND_ADDRESS 0x08000004 // pysical address

static int result;
static dev_t inter_dev;
static struct cdev inter_cdev;
static unsigned char *iom_fpga_fnd_addr;

static int stopwatch_open(struct inode *, struct file *);
static int stopwatch_release(struct inode *, struct file *);
static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);

struct tasklet_struct start_tasklet;
struct tasklet_struct stop_tasklet;

s64 pushed_time;

int i;
int count = 0;
int finish_count = 0;
volatile int isStarted = 0;
volatile int isFinished = 0;
unsigned char fnd_data[4];


wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

static struct file_operations inter_fops =
{
	.open = stopwatch_open,
	.write = stopwatch_write,
	.release = stopwatch_release,
};



// when write to fnd device  ,call this function
void set_fnd() 
{
	unsigned short int value_short = 0;
    value_short = fnd_data[0] << 12 | fnd_data[1] << 8 |fnd_data[2] << 4 |fnd_data[3];
    outw(value_short,(unsigned int)iom_fpga_fnd_addr);	    
}

void reset_fnd(){
    for (i = 0; i < 4; i++)
    {
        fnd_data[i] = 0;
    }
    set_fnd();
	printk("reset fnd\n");
}

void update_fnd() {z 
	if(fnd_data[3] >= 9) {
		fnd_data[3] = 0;
		if(fnd_data[2] >= 5) {
			fnd_data[2] =0;
			if(fnd_data[1] >= 9) {
				fnd_data[1] = 0;
				if(fnd_data[0] >= 5) {
					fnd_data[0] = 0;
				}
				else {
					fnd_data[0] +=1;
				}
			}
			else {
				fnd_data[1] +=1;
			}
		}
		else {
			fnd_data[2] +=1;
		}
	}
	else {
		fnd_data[3] +=1;
	}
	set_fnd();
}


void start(unsigned long unsued) {
	count++;
	printk(KERN_ALERT "count : %d\n", count);	
	if(count == 10) {
		update_fnd();
	}
	mdelay(100);
	if(isStarted) {
		tasklet_schedule(&start_tasklet);
	}
}

//Home
irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt1!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	isStarted = 1;
	tasklet_schedule(&start_tasklet);
	return IRQ_HANDLED;
}

//BACK
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) {
        printk(KERN_ALERT "interrupt2!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
        isStarted = 0;
		return IRQ_HANDLED;
}


// Vol+
irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
        printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
        isStarted = 0;
		count = 0;
		reset_fnd();
		return IRQ_HANDLED;
}

void stop(unsigned long unused) {
	if (pushed_time + 3000000000 <= ktime_to_ns(ktime_get())) {
		isStarted = 0;
		count = 0;
		reset_fnd();
		__wake_up(&wq_write, 1, 1, NULL);
	}
	if(!gpio_get_value(IMX_GPIO_NR(5, 14))) {
		tasklet_schedule(&stop_tasklet);
	}
}

// Vol-
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt4!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	pushed_time = ktime_to_ns(ktime_get());
	tasklet_schedule(&stop_tasklet);
	return IRQ_HANDLED; 
}


static int stopwatch_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	printk(KERN_ALERT "Open Module\n");

	// int1
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler2, IRQF_TRIGGER_FALLING, "back", 0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING, "voldown", 0);

	// set fnd
	reset_fnd();

	tasklet_init(&start_tasklet, start, 0);
	tasklet_init(&pause_tasklet, pause, 0);
	tasklet_init(&reset_tasklet, reset, 0);
	tasklet_init(&stop_tasklet, stop, 0);

	
	return 0;
}

static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
    printk("sleep on\n");
    interruptible_sleep_on(&wq_write);
	printk("write\n");
	return 0;
}

static int stopwatch_release(struct inode *minode, struct file *mfile){
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	
	printk(KERN_ALERT "Release Module\n");
	return 0;
}

static int stopwatch_register_cdev(void)
{
	int error;
    inter_dev = MKDEV(IOM_FND_MAJOR, 0);
    error = register_chrdev_region(inter_dev,1,"stopwatch");
	if(error<0) {
		printk(KERN_WARNING "inter: can't get major %d\n", IOM_FND_MAJOR);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", IOM_FND_MAJOR);
	cdev_init(&inter_cdev, &inter_fops);
	inter_cdev.owner = THIS_MODULE;
	inter_cdev.ops = &inter_fops;
	error = cdev_add(&inter_cdev, inter_dev, 1);
	if(error)
	{
		printk(KERN_NOTICE "inter Register Error %d\n", error);
	}
	return 0;
}

static int __init stopwatch_init(void) {
	int result;
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
	if((result = stopwatch_register_cdev()) < 0 )
		return result;
	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/%s, Major Num :%d\n", IOM_FND_NAME, IOM_FND_MAJOR);
	return 0;
}

static void __exit stopwatch_exit(void) {
	tasklet_disable(&start_tasklet);
	tasklet_disable(&stop_tasklet);

	cdev_del(&inter_cdev);
    iounmap(iom_fpga_fnd_addr);
	unregister_chrdev_region(inter_dev, 1);
	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);
MODULE_LICENSE("GPL");
