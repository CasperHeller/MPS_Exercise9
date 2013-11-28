#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include "linux/cpld-gpio.h"
#include <linux/timer.h>

#define R1_GPIO 136
#define R2_GPIO 131
#define R3_GPIO 133
#define R4_GPIO 135
#define C1_GPIO 130
#define C2_GPIO 132
#define C3_GPIO 134

struct input_dev* vms_input_dev;
static struct platform_device* vms_dev;
// Tilføjer timer
struct timer_list gpioTimer;
int timeout_in_msec = 100;
int index = 1;


static void moveCursor(int X_SENS, int Y_SENS)
{
    input_report_rel(vms_input_dev, REL_X, X_SENS);
    input_report_rel(vms_input_dev, REL_Y, Y_SENS);

    input_sync(vms_input_dev);
    printk("x = %d, y = %d\n", X_SENS, Y_SENS);
}

//Interrupt handler
static irqreturn_t IRQ_Handler(int irq, void *dev_id)
{
    if(!cpld_gpio_get_value(C1_GPIO))
    {
      if (!cpld_gpio_get_value(R1_GPIO))
      {
	printk("R1 - C1\n");
      }
      
      if (!cpld_gpio_get_value(R2_GPIO))
      {
	printk("R2 - C1\n");
      }
      
      if (!cpld_gpio_get_value(R3_GPIO))
      {
	printk("R3 - C1\n");
	moveCursor(30, 0);
      }
      
      if (!cpld_gpio_get_value(R4_GPIO))
      {
	printk("R4 - C1\n");
      }
    }
    
    if(!cpld_gpio_get_value(C2_GPIO))
    {
      if (!cpld_gpio_get_value(R1_GPIO))
      {
	printk("R1 - C2\n");
      }
      
      if (!cpld_gpio_get_value(R2_GPIO))
      {
	printk("R2 - C2\n");
	moveCursor(0, 30);
      }
      
      if (!cpld_gpio_get_value(R3_GPIO))
      {
	printk("R3 - C2\n");
	moveCursor(0, -30);
      }
      
      if (!cpld_gpio_get_value(R4_GPIO))
      {
	printk("R4 - C2\n");
      }
    }
    
    if(!cpld_gpio_get_value(C3_GPIO))
    {
      if (!cpld_gpio_get_value(R1_GPIO))
      {
	printk("R1 - C3\n");
      }
      
      if (!cpld_gpio_get_value(R2_GPIO))
      {
	printk("R2 - C3\n");
      }
      
      if (!cpld_gpio_get_value(R3_GPIO))
      {
	printk("R3 - C3\n");
	moveCursor(0, 30);
      }
      
      if (!cpld_gpio_get_value(R4_GPIO))
      {
	printk("R4 - C3\n");
      }
    }    
   
    return IRQ_HANDLED;
}

static void timerFunc(unsigned long arg)
{
    gpioTimer.expires = jiffies + timeout_in_msec*(HZ/1000);
    add_timer(&gpioTimer);
    
    cpld_gpio_set_value(C1_GPIO, 1);
    cpld_gpio_set_value(C2_GPIO, 1);
    cpld_gpio_set_value(C3_GPIO, 1);
    
    switch(index)
    {
    case 1:
        cpld_gpio_set_value(C1_GPIO, 0);
	break;
    case 2:
	cpld_gpio_set_value(C2_GPIO, 0);
        break;
    case 3:
	cpld_gpio_set_value(C3_GPIO, 0);
        break;
    }

    index++;

    if(index > 3)
    {
        index = 1;
    }
}

static ssize_t write_vms(struct device* dev,
                         struct device_attribute* attr,
                         const char* buffer, size_t count)
{
   int x,y;
   sscanf(buffer, "%d%d", &x, &y);
   
   input_report_rel(vms_input_dev, REL_X, x);
   input_report_rel(vms_input_dev, REL_Y, y);

   input_sync(vms_input_dev);
   printk("x = %d, y = %d\n", x, y);
   
   return count;
}

DEVICE_ATTR(coordinates, 0666, NULL, write_vms);

static struct attribute* vms_attr[] = 
{
   &dev_attr_coordinates.attr,
   NULL
};

static struct attribute_group vms_attr_group = 
{
   .attrs = vms_attr,
};


static int vms_open(struct input_dev *dev)
{
    return 0;
}

static void vms_close(struct input_dev *dev)
{
}

int __init vms_init(void)
{
   int err = 0;
  
   vms_dev = platform_device_register_simple("vms", -1, NULL, 0);
   if(IS_ERR(vms_dev))
   {
      PTR_ERR(vms_dev);
      printk("vms_init: error\n");
   }
   
   sysfs_create_group(&vms_dev->dev.kobj, &vms_attr_group);
   
   vms_input_dev = input_allocate_device();

   if(!vms_input_dev)
   {
      printk("Bad input_alloc_device()\n");
   }

   vms_input_dev->name = "Test";

        /*
   vms_input_dev->name = "vms_input";
   vms_input_dev->open  = vms_open;
   vms_input_dev->close = vms_close;
        */

   //   vms_input_dev->id.bustype = BUS_HOST;
   
   set_bit(EV_REL, vms_input_dev->evbit);   
   set_bit(REL_X, vms_input_dev->relbit);
   set_bit(REL_Y, vms_input_dev->relbit);

   input_set_capability(vms_input_dev, EV_KEY, BTN_LEFT);
   input_set_capability(vms_input_dev, EV_KEY, BTN_MIDDLE);
   input_set_capability(vms_input_dev, EV_KEY, BTN_RIGHT);

   input_register_device(vms_input_dev);
   
   printk("Virtual Mouse Driver Initialized.\n");
  
   
    if ((err = cpld_gpio_request(R1_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting R1_GPIO: %d\n", err);
    }

    if ((err = cpld_gpio_request(R2_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting R2_GPIO: %d\n", err);
    }

    if ((err = cpld_gpio_request(R3_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting R3_GPIO: %d\n", err);
    }

    if ((err = cpld_gpio_request(R4_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting R4_GPIO: %d\n", err);
    }

    if ((err = cpld_gpio_request(C1_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting COULMN2_GPIO: %d\n", err);
    }

    if ((err = cpld_gpio_request(C2_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting C2_GPIO: %d\n", err);
    }

    if ((err = cpld_gpio_request(C3_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting C3_GPIO: %d\n", err);
    }

    if((err = cpld_gpio_direction_input(R1_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error setting R1_GPIO direction: %d\n", err);
    }

    if((err = cpld_gpio_direction_input(R2_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error setting R2_GPIO direction: %d\n", err);
    }

    if((err = cpld_gpio_direction_input(R3_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error setting R3_GPIO direction: %d\n", err);
    }

    if((err = cpld_gpio_direction_input(R4_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error setting R4_GPIO direction: %d\n", err);
    }

    if((err = cpld_gpio_direction_output(C1_GPIO, 1)) < 0)
    {
	printk(KERN_ALERT "Error setting C1_GPIO direction: %d\n", err);
    }

    if((err = cpld_gpio_direction_output(C2_GPIO, 1)) < 0)
    {
	printk(KERN_ALERT "Error setting C2_GPIO direction: %d\n", err);
    }

    if((err = cpld_gpio_direction_output(C3_GPIO, 1)) < 0)
    {
	printk(KERN_ALERT "Error setting C3 direction: %d\n", err);
    }
    //
    
    cpld_gpio_set_value(C1_GPIO, 1);
    cpld_gpio_set_value(C2_GPIO, 1);
    cpld_gpio_set_value(C3_GPIO, 1);
    
    unsigned int IRQ1 = cpld_gpio_to_irq(R1_GPIO);
    unsigned int IRQ2 = cpld_gpio_to_irq(R2_GPIO);
    unsigned int IRQ3 = cpld_gpio_to_irq(R3_GPIO);
    unsigned int IRQ4 = cpld_gpio_to_irq(R4_GPIO);

    if((err = request_irq(IRQ1,
				  IRQ_Handler,
				  IRQF_TRIGGER_RISING,
				  "R1_INTERRUPT",
				  R1_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting interrupt R1: %d\n", err);
    }

    if((err = request_irq(IRQ2,
				  IRQ_Handler,
				  IRQF_TRIGGER_RISING,
				  "R2_INTERRUPT",
				  R2_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting interrupt R2: %d\n", err);
    }

    if((err = request_irq(IRQ3,
				  IRQ_Handler,
				  IRQF_TRIGGER_RISING,
				  "R3_INTERRUPT",
				  R3_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting interrupt R3: %d\n", err);
    }

    if((err = request_irq(IRQ4,
				  IRQ_Handler,
				  IRQF_TRIGGER_RISING,
				  "R4_INTERRUPT",
				  R4_GPIO)) < 0)
    {
	printk(KERN_ALERT "Error requesting interrupt R4: %d\n", err);
    }
  
    //Start timer
    init_timer(&gpioTimer);
    gpioTimer.expires = jiffies + timeout_in_msec*(HZ/1000);
    gpioTimer.function =  timerFunc;
    gpioTimer.data = 0;
    add_timer(&gpioTimer);
    //
  
    return 0;
}

void __exit vms_cleanup(void)
{
   input_unregister_device(vms_input_dev);
   
   sysfs_remove_group(&vms_dev->dev.kobj, &vms_attr_group);
   
   platform_device_unregister(vms_dev);
   
   //Free GPIOs & IRQs
   //free_irq(IRQ1);
   //free_irq(IRQ2);
   //free_irq(IRQ3);
   //free_irq(IRQ4);
   
   // Frigiver GPIOs
   cpld_gpio_free(R1_GPIO);
   cpld_gpio_free(R2_GPIO);
   cpld_gpio_free(R3_GPIO);
   cpld_gpio_free(R4_GPIO);
   cpld_gpio_free(C1_GPIO);
   cpld_gpio_free(C2_GPIO);
   cpld_gpio_free(C3_GPIO);
   
   printk("EXIT FINISHED!");

   return;
}


module_init(vms_init);
module_exit(vms_cleanup);

MODULE_AUTHOR("Danish Technological Institut");
MODULE_LICENSE("GPL");
