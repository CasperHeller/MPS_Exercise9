#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/cpld-gpio.h>
#include "cpld-core.h"


enum bool {
  FALSE=0,
  TRUE=1
};

enum cpld_gpio_dir {
  IN,
  OUT
};

#define CPLD_GPIO_MIN 130
#define CPLD_GPIO_MAX 137
#define CPLD_GPIO_DEFAULT_INPUT_START 134
#define CPLD_GPIO_CNT (CPLD_GPIO_MAX-CPLD_GPIO_MIN+1) 
#define CPLD_GPIO_TO_AL_IDX(gpio) (gpio-CPLD_GPIO_MIN)
#define CPLD_GPIO_VALID(gpio) (( gpio >= CPLD_GPIO_MIN && gpio <= CPLD_GPIO_MAX ) ? TRUE : FALSE)
#define CPLD_GPIO_ADDR 0x02
#define CPLD_GPIO_DIR(gpio) ( (  gpio_dirs & (1 << (gpio-CPLD_GPIO_MIN)) ) ? IN : OUT)


static DEFINE_MUTEX(access_lock);
static unsigned gpio_dirs = 0;
static unsigned isRegistered = 0;

struct cpld_gpio_access_list_t 
{
  enum bool in_use;
};


static struct cpld_gpio_access_list_t al[CPLD_GPIO_CNT];


/* [gpio_begin, gpio_end( */
static void cpld_gpio_free_range(unsigned gpio_begin, unsigned gpio_end)
{
  int i = gpio_begin;

  printk(KERN_INFO "Freeing gpios from %d to %d\n", gpio_begin, gpio_end-1);
  for(; i < gpio_end; ++i)
  {
    gpio_free(i);
  }
}


int cpld_gpio_register(void)
{
  int result = 0;
  int i = 0;


  if(isRegistered == FALSE)
  {
    printk(KERN_INFO "Registering gpios\n");
    isRegistered = TRUE;
    
    for(; i < CPLD_GPIO_CNT; ++i)    
    {
      result = gpio_request(CPLD_GPIO_MIN+i, "cpld-gpio");
      if(result != 0)
        goto release_gpios;
      
    }

  }
  else
    printk(KERN_INFO "Reregistering gpios\n");

  return 0;

  release_gpios:
  cpld_gpio_free_range(CPLD_GPIO_MIN, i);
  return result;
}


int cpld_gpio_is_registered(void)
{
  return isRegistered;
}


int cpld_gpio_set_defaults(void)
{
  int     i = 0;
  gpio_dirs = 0;
  
  for(; i < CPLD_GPIO_CNT; ++i)    
  {
    al[i].in_use = FALSE;
    if( (CPLD_GPIO_MIN+i) < CPLD_GPIO_DEFAULT_INPUT_START)
    {
      printk(KERN_INFO "Setting gpio %d as input\n", CPLD_GPIO_MIN+i);
      
      gpio_direction_input(CPLD_GPIO_MIN+i);
      gpio_dirs |= 1 << i;
    }
    else
    {
      printk(KERN_INFO "Setting gpio %d as output\n", CPLD_GPIO_MIN+i);
      
      gpio_direction_output(CPLD_GPIO_MIN+i,0);
    }
    
  }

  cpld_write(CPLD_GPIO_ADDR, gpio_dirs);

  return 0;
}


void cpld_gpio_unregister(void)
{
  if(isRegistered == TRUE)
  {
    printk(KERN_INFO "Unregistering gpios\n");
    isRegistered = FALSE;
    cpld_gpio_free_range(CPLD_GPIO_MIN, CPLD_GPIO_MAX+1);
  }
  else
    printk(KERN_INFO "Reunregistering gpios\n");
}


int cpld_gpio_init(void)
{
  // acquire gpio range 130-137 and name it cpld-addon
  int result = 0;
  int i = 0;

  for(; i < CPLD_GPIO_CNT; ++i)    
  {
    al[i].in_use = FALSE;
  }
  
  result = cpld_gpio_register();
  if(result != 0)
    goto done_or_error;

  result = cpld_gpio_set_defaults();

  cpld_gpio_unregister();
  
  done_or_error:
  return result;  
}


void cpld_gpio_exit(void)
{
  // Free said gpio range
  cpld_gpio_unregister();
}

int cpld_gpio_request(unsigned gpio)
{
  int status = -ECHRNG;
  // acquire in internal to cpld-addon driver access list
  if(!CPLD_GPIO_VALID(gpio))
    return status;

  if(isRegistered == FALSE)
  {
    // If not registered then plz do so before use...
      int res = cpld_gpio_register();
      if(res)
        return res;
      res = cpld_gpio_set_defaults();
      if(res)
        return res;
  }
  

  
  mutex_lock(&access_lock);
  
  if(!al[CPLD_GPIO_TO_AL_IDX(gpio)].in_use)
  {
    al[CPLD_GPIO_TO_AL_IDX(gpio)].in_use = TRUE;
    status = 0;
  }
  else
    status = -EBUSY;
  
  mutex_unlock(&access_lock);

  return status;
}
EXPORT_SYMBOL_GPL(cpld_gpio_request);


static int _cpld_gpio_direction_input(unsigned gpio)
{
  int status = -ECHRNG;
  // acquire in internal to cpld-addon driver access list
  if(!CPLD_GPIO_VALID(gpio))
    return status;

  if(al[CPLD_GPIO_TO_AL_IDX(gpio)].in_use)
  {
    if(CPLD_GPIO_DIR(gpio) == OUT)
    {
      gpio_direction_input(gpio);
      gpio_dirs |= 1 << (gpio - CPLD_GPIO_MIN);
      cpld_write(CPLD_GPIO_ADDR, gpio_dirs);
    }
    else
    {
      // Do nothing
    }
    status = 0;
  }
  else
    status = -EBADR;
  
  return status;
}


int cpld_gpio_direction_input(unsigned gpio)
{
  int status = -ECHRNG;

  mutex_lock(&access_lock);
  
  status = _cpld_gpio_direction_input(gpio);
  
  mutex_unlock(&access_lock);

  return status;
}
EXPORT_SYMBOL_GPL(cpld_gpio_direction_input);


int cpld_gpio_direction_output(unsigned gpio, int value)
{
  int status = -ECHRNG;
  // acquire in internal to cpld-addon driver access list
  if(!CPLD_GPIO_VALID(gpio))
    return status;

  mutex_lock(&access_lock);
  
  if(al[CPLD_GPIO_TO_AL_IDX(gpio)].in_use)
  {
    if(CPLD_GPIO_DIR(gpio) == IN)
    {
      gpio_dirs &= ~(1 << (gpio - CPLD_GPIO_MIN));
      cpld_write(CPLD_GPIO_ADDR, gpio_dirs);
      gpio_direction_output(gpio, value);
    }
    else
    {
      // Do nothing
    }
    status = 0;    
  }
  else
    status = -EBADR;
  
  mutex_unlock(&access_lock);

  return status;
}
EXPORT_SYMBOL_GPL(cpld_gpio_direction_output);


int cpld_gpio_get_value(unsigned gpio)
{
  return gpio_get_value(gpio);
}
EXPORT_SYMBOL_GPL(cpld_gpio_get_value);


void cpld_gpio_set_value(unsigned gpio, int value)
{
  return gpio_set_value(gpio, value);
}
EXPORT_SYMBOL_GPL(cpld_gpio_set_value);


int cpld_gpio_to_irq(unsigned gpio)
{
  return gpio_to_irq(gpio);
}
EXPORT_SYMBOL_GPL(cpld_gpio_to_irq);


int cpld_gpio_irq_to_gpio(unsigned irq)
{
  return irq_to_gpio(irq);
}
EXPORT_SYMBOL_GPL(cpld_gpio_irq_to_gpio);

void cpld_gpio_free(unsigned gpio)
{
  // acquire in internal to cpld-addon driver access list
  if(!CPLD_GPIO_VALID(gpio))
    return;
  
  mutex_lock(&access_lock);
  
  if(al[CPLD_GPIO_TO_AL_IDX(gpio)].in_use)
  {
    _cpld_gpio_direction_input(gpio); // Set default input
    al[CPLD_GPIO_TO_AL_IDX(gpio)].in_use = FALSE;
  }
  else
  {
    // Do nothing
  }
  
  mutex_unlock(&access_lock);
}
EXPORT_SYMBOL_GPL(cpld_gpio_free);
