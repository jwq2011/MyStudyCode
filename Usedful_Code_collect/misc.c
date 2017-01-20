#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/delay.h>   
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <mach/cache_operation.h> 
#include <asm/system.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <mach/ac83xx_gpio_pinmux.h>
#include <mach/ac83xx_gpio_pinmux_mapping.h>


#define ATC_KERNEL_LINUX_LICENSE     "GPL"

int phone_mic_switch(bool state);

typedef struct mic_device {
		int val;
		struct semaphore sem;
        struct cdev dev;
}mic_device;

//主设备号和从设备号
static int mic_major = 0;
static int mic_minor = 0;

//设备类别和设备变量
static struct class *mic_class = NULL;
static struct mic_device *mic_dev = NULL;

static struct mic_device *g_micdev = NULL;

extern int gpio_configure(unsigned gpio, int dir, int value);
extern int gpio_request(unsigned gpio, const char *label);
extern void gpio_free(unsigned gpio);
extern int gpio_direction_output(unsigned gpio, int value);

/*
struct mic_hardware{
	int (*mic_switch)(bool);
};

struct mic_hardware mic_hw = {
	.mic_switch = phone_mic_switch,
};


int phone_mic_switch(bool state)
{
	int err = 0;

   	if(state)
   	{
   		gpio_request(MIC_SWITCH_PIN, "output");
		gpio_direction_output(MIC_SWITCH_PIN,0);  //4G module mic
		gpio_free(MIC_SWITCH_PIN);
   	}
	else
	{
		gpio_request(MIC_SWITCH_PIN, "output");
		gpio_direction_output(MIC_SWITCH_PIN,1);  //BT mic
		gpio_free(MIC_SWITCH_PIN);
	}

   return err;
}
*/

/****************************************传统方式**********************/
//#define PERFTEST
static int mic_ioctl( struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

	printk(KERN_INFO "mic ioctl: command = %u,param = %x \n", cmd,(int)arg); 

	switch(cmd)
	{
//		case BTDRV_IOCTL_POWER_OFF:
//			bt_ext_power_off(1);		
//			break;
		default:
			return - EINVAL;
	}
    return err;
}

static int mic_open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "mic_open\n");

	struct mic_device* dev;
	//将结构体变量保存到私有区域
	dev = container_of(inode->i_cdev, struct mic_device, dev);
	filp->private_data = dev;

    return 0;
}

static int mic_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mic_release\n");

    return 0;
}

static ssize_t mic_read( struct file* flip, char __user* buf, size_t count, loff_t* f_pos)
{
	ssize_t err = 0;
	struct mic_device* dev = flip->private_data;
	//锁住信号量，以保证同步访问
	if(down_interruptible( &(dev->sem) ))
	{
		return -ERESTARTSYS ;
	}
	if(count < sizeof(dev->val) )
	{
		goto out;
	}
	// 将数据拷贝到用户提供的缓存区中
	if(__copy_to_user(buf, &(dev ->val), sizeof(dev->val) ))
	{
		err = -EFAULT;
		goto out;
	}
	err = sizeof(dev->val);
	out:
		//释放同步信号
		up(&(dev->sem));
	printk(KERN_INFO "mic_read\n");
	return err;
}

static ssize_t mic_write(struct file* flip,  const char __user* buf, size_t count, loff_t* f_pos)
{
	struct mic_device* dev = flip->private_data;
	ssize_t err = 0;
	//锁住信号量，以便进行同步访问
	if(down_interruptible( &(dev->sem) ))
	{
		return -ERESTARTSYS;
	}
	if(count != sizeof(dev->val) )
	{
		goto out;
	}
	//从用户提供的缓存中获取相关数据
	if(__copy_from_user( &(dev->val), buf, count) )
	{
		err = -EFAULT;
		goto out;
	}

	if(dev->val == 0)
   	{
   		gpio_request(MIC_SWITCH_PIN, "output");
		gpio_direction_output(MIC_SWITCH_PIN,0);  //4G module mic
		gpio_free(MIC_SWITCH_PIN);
		printk(KERN_INFO "switch to 4G module mic\n");
   	}
	else if(dev->val == 1)
	{
		gpio_request(MIC_SWITCH_PIN, "output");
		gpio_direction_output(MIC_SWITCH_PIN,1);  //BT mic
		gpio_free(MIC_SWITCH_PIN);
		printk(KERN_INFO "switch to BT mic\n");
	}
	else if(dev->val == 2)   //reset 4G module
	{
		gpio_request(RESET_4G_MODULE, "output");
		gpio_direction_output(RESET_4G_MODULE,0);
		msleep(100);
		gpio_direction_output(RESET_4G_MODULE,1);
		gpio_free(RESET_4G_MODULE);
		printk(KERN_INFO "reset 4G module\n");
	}
	else if(dev->val == 3)	//usb fast charge mode
	{
		gpio_request(USB_FAST_CHARGE_SWITCH,"output");
		gpio_direction_output(USB_FAST_CHARGE_SWITCH,0);
		gpio_free(USB_FAST_CHARGE_SWITCH);
		printk(KERN_INFO "USB_FAST_CHARGE_SWITCH GPIO5 is low\n");
	}
	else if(dev->val == 4)
	{
		gpio_request(USB_FAST_CHARGE_SWITCH,"output");
		gpio_direction_output(USB_FAST_CHARGE_SWITCH,1);
		gpio_free(USB_FAST_CHARGE_SWITCH);
		printk(KERN_INFO "USB_FAST_CHARGE_SWITCH GPIO5 is high\n");
	}
	
	err = sizeof(dev->val);
	out:
		up(&(dev->sem));
		printk(KERN_INFO "mic_write\n");
	return err;
}

static struct file_operations mic_fops = {
    .owner          = THIS_MODULE,
    .open           = mic_open,
    .release        = mic_release,
    .read   		= mic_read,
    .write			= mic_write,
    .unlocked_ioctl = mic_ioctl,
};


/***********************************devfs文件系统方式*******************************/
//devfs文件系统的设备属性操作方法
static ssize_t mic_val_show( struct device* dev, struct device_attribute* attr,  char* buf);
static ssize_t mic_val_store( struct device* dev, struct device_attribute* attr,  const char* buf, size_t count);
//devfs文件系统的设备属性
static DEVICE_ATTR(val, S_IRUGO | S_IWUSR, mic_val_show, mic_val_store);

// 将myfake_android_dev.val数据拷贝到用户的缓冲区中，仅供内部使用
static ssize_t __mic_get_val( struct mic_device* dev,  char* buf)
{
	int val = 0;
	//同步访问
	if(down_interruptible( &(dev->sem) ))
	{
	return -ERESTARTSYS;
	}
	val = dev->val;
	up( &(dev->sem) );
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

//将用户提供的数据写入到myfake_android_dev.val变量中，仅供内部使用
static ssize_t __mic_set_val( struct mic_device* dev, const char* buf, size_t count)
{
	int val = 0;
	//字符串转为数字
	val = simple_strtol(buf, NULL, 10);
	//同步访问
	if(down_interruptible( &(dev->sem) ))
	{
	return -ERESTARTSYS;
	}
	dev->val = val;
	if(dev->val == 0)
   	{
   		gpio_request(MIC_SWITCH_PIN, "output");
		gpio_direction_output(MIC_SWITCH_PIN,0);  //4G module mic
		gpio_free(MIC_SWITCH_PIN);
		printk(KERN_INFO "switch to 4G module mic\n");
   	}
	else if(dev->val == 1)
	{
		gpio_request(MIC_SWITCH_PIN, "output");
		gpio_direction_output(MIC_SWITCH_PIN,1);  //BT mic
		gpio_free(MIC_SWITCH_PIN);
		printk(KERN_INFO "switch to BT mic\n");
	}
	else if(dev->val == 2)
	{
		gpio_request(RESET_4G_MODULE, "output");
		gpio_direction_output(RESET_4G_MODULE,0);
		msleep(100);
		gpio_direction_output(RESET_4G_MODULE,1);
		gpio_free(RESET_4G_MODULE);
		printk(KERN_INFO "reset 4G module\n");
	}
	else if(dev->val == 3)  //usb fast charge mode
	{
		gpio_request(USB_FAST_CHARGE_SWITCH,"output");
		gpio_direction_output(USB_FAST_CHARGE_SWITCH,0);
		gpio_free(USB_FAST_CHARGE_SWITCH);
		printk(KERN_INFO "USB_FAST_CHARGE_SWITCH GPIO5 is low\n");
	}
	else if(dev->val == 4)
	{
		gpio_request(USB_FAST_CHARGE_SWITCH,"output");
		gpio_direction_output(USB_FAST_CHARGE_SWITCH,1);
		gpio_free(USB_FAST_CHARGE_SWITCH);
		printk(KERN_INFO "USB_FAST_CHARGE_SWITCH GPIO5 is high\n");
	}

	
	up( &(dev->sem));
	return count;
}

//devfs文件系统的读接口
static ssize_t mic_val_show( struct device* dev, struct device_attribute* attr, char* buf)
{
	struct mic_device* hdev = ( struct mic_device*)dev_get_drvdata(dev);
	printk(KERN_INFO "mic_val_show \n");
	return __mic_get_val(hdev, buf);
}
//devfs文件系统的写接口
static ssize_t mic_val_store( struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct mic_device* hdev = ( struct mic_device*)dev_get_drvdata(dev);
	printk(KERN_INFO "mic_val_store\n");
	return __mic_set_val(hdev, buf, count);
}


/*
static int __init mic_init(void);

static int __devinit mic_probe(struct platform_device *pdev)
{
    struct mic_device *micdev;
    int result;
    
    micdev = kzalloc(sizeof(struct mic_device), GFP_KERNEL);
    if (micdev == NULL) {
        dev_err(&pdev->dev, "[mic]: mic_probe: malloc device failed\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, micdev);
 
    micdev->cdev.name = "mic";
    micdev->cdev.minor = MISC_DYNAMIC_MINOR;
    micdev->cdev.fops = &mic_fops;
    micdev->dev = &(pdev->dev);
    micdev->cdev.parent = &(pdev->dev);

    result = misc_register(&(micdev->cdev));

    if ( result == 0 )
    {
        printk(KERN_INFO "mic init successes\n");
    } 
    else 
    {
        printk(KERN_ERR "mic misc device register error\n");
        kfree(micdev);
        return result;
    }
    g_micdev = micdev;

    return 0;
}

static int __devexit mic_remove(struct platform_device *pdev)
{   
    struct mic_device *micdev = platform_get_drvdata(pdev);
    
    if (micdev == NULL)
    {
        printk(KERN_ERR "No device when bkl remove!\n");
        return -ENODEV;
    }

    misc_deregister(&(micdev->cdev));
    
    kfree(g_micdev);
    g_micdev = NULL;
    
    return 0;
}

static struct platform_driver mic_drv = {
    .driver = {
        .name = "mic",
        .owner = THIS_MODULE, 
            },
    .probe  = mic_probe,
    .remove = __devexit_p(mic_remove),
};

static void mic_dev_release(struct device *dev)
{
    printk("mic_dev_release called!\n");
}   


static struct platform_device mic_dev = {
    .name = "mic",
    .id = 0,
    .dev ={
    	.platform_data = &mic_hw,
    	},
    .dev.release = mic_dev_release,
};
*/

#if 1
#include <linux/proc_fs.h>
#include <linux/version.h>

#define MISC_DIR_NAME        "misc_proc"
#define MISC_ENTRY_NAME		"dev"

#define SET_ARM_GPIO_PIN			0x80
#define GPIO_LCD_SIGN				PIN_3_GPIO3
#ifdef PRINT_DEBUG
#define DEBUG_INFO(fmt, arg...)  printk("<--MISC-->"fmt, ##arg)
#else
#define DEBUG_INFO(fmt, arg...)
#endif

static struct proc_dir_entry *misc_dir = NULL;
static struct proc_dir_entry *misc_entry = NULL;
static struct mic_device *misc_dev = NULL;
static unsigned char mem_tmp[5] = {0};
struct semaphore misc_sem;


static int misc_ioctl( struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

	DEBUG_INFO(KERN_INFO "misc ioctl: command = %u,param = %x \n", cmd,(int)arg);

	switch(cmd)
	{
		default:
			return - EINVAL;
	}
    return err;
}

static int misc_open(struct inode *inode, struct file *filp)
{
    DEBUG_INFO(KERN_INFO "misc_open\n");

    return 0;
}

static int misc_release(struct inode *inode, struct file *file)
{
    DEBUG_INFO(KERN_INFO "misc_release\n");

    return 0;
}

static ssize_t misc_read( struct file* flip, char __user* buf, size_t count, loff_t* f_pos)
{
	ssize_t err = 0;
	//锁住信号量，以保证同步访问
	if(down_interruptible( &misc_sem))
	{
		return -ERESTARTSYS ;
	}

	// 将数据拷贝到用户提供的缓存区中
	if(copy_to_user(buf, mem_tmp, count ))
	{
		err = -EFAULT;
		goto out;
	}
	err = count;
	out:
		//释放同步信号
		up(&misc_sem);
	DEBUG_INFO(KERN_INFO "misc_read\n");
	return err;
}

static ssize_t misc_write(struct file* flip,  const char __user* buf, size_t count, loff_t* f_pos)
{
	ssize_t err = 0, cmd_type = 0;
	//锁住信号量，以便进行同步访问
	if(down_interruptible(&misc_sem))
	{
		return -ERESTARTSYS;
	}

	//从用户提供的缓存中获取相关数据
	if(copy_from_user( mem_tmp, buf, count) )
	{
		err = -EFAULT;
		goto out;
	}

	cmd_type = mem_tmp[0];
	switch(cmd_type)
	{
		case SET_ARM_GPIO_PIN:
			gpio_request(mem_tmp[1], "output");
			gpio_direction_output(mem_tmp[1], (mem_tmp[2])?1:0);
			gpio_free(mem_tmp[1]);
			DEBUG_INFO(KERN_INFO "Set GPIO %d %s\n", mem_tmp[1], (mem_tmp[2])?"Hight":"LOW");
			break;
		default:
			break;
	}

	err = count;
	out:
		up(&misc_sem);
		DEBUG_INFO(KERN_INFO "misc_write\n");
	return err;
}

static struct file_operations misc_fops = {
    .owner          = THIS_MODULE,
    .open           = misc_open,
    .release        = misc_release,
    .read   		= misc_read,
    .write			= misc_write,
    .unlocked_ioctl = misc_ioctl,
};

static int misc_setup_proc(void)
{
	misc_dir = proc_mkdir(MISC_DIR_NAME, NULL);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)/* 从3.10内核后架构有转变 */
	misc_entry = create_proc_entry(MISC_ENTRY_NAME, 0666, misc_dir);
	misc_entry->proc_fops = &misc_fops;
#else
	misc_entry = proc_create(MISC_ENTRY_NAME, 0666, misc_dir, &misc_fops);
#endif
	init_MUTEX(&misc_sem);

	return 0;
}
#endif

//初始化设备
static int __mic_setup_dev(struct mic_device* dev)
{
	int err;
	dev_t devno = MKDEV(mic_major, mic_minor);
	memset(dev, 0, sizeof(struct mic_device) );
	cdev_init( &(dev ->dev), &mic_fops);
	//dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &mic_fops;
	//注册字符设备
	err = cdev_add( &(dev ->dev), devno, 1);
	if(err)
	{
		return err;
	}
	//初始化信号量
	init_MUTEX(&(dev ->sem));
	dev->val = 0;
	return 0;
}

static int __init mic_init(void)
{
    int ret = -1;
	dev_t dev = 0;
	struct device* temp = NULL;
	
	printk(KERN_ALERT"Initializing mic device. \n");
/*
    ret = platform_device_register(&mic_dev);
    if (ret) {
        printk(KERN_ERR "[mic]: %s: register  device failed\n", __func__); 
        goto fail0;
    }
    
    ret = platform_driver_register(&mic_drv);
    if (ret) {
        printk(KERN_ERR "[mic]: %s: register  driver failed\n", __func__);
        goto fail1;
    }   
*/
	//分配设备号码
	ret = alloc_chrdev_region( &dev, 0, 1, "mic");
	if(ret < 0)
	{
		printk(KERN_ALERT"Failed to alloc char dev region. \n");
		goto fail0;
	}
	mic_major = MAJOR(dev);
	mic_minor = MINOR(dev);
	// 分配设备结构体的空间
	mic_dev = kmalloc( sizeof(struct mic_device), GFP_KERNEL);
	if(!mic_dev)
	{
		ret = -ENOMEM;
		printk(KERN_ALERT"Failed to alloc myfake_dev. \n");
		goto unregister;
	}
	//初始化设备
	ret = __mic_setup_dev(mic_dev);
	if(ret)
	{
		printk(KERN_ALERT"Failed to setup dev:%d.\n", ret);
		goto cleanup;
	}

	//创建传统设备文件系统的接口设备
	// create device type directory myfake on /sys/ class/
	mic_class = class_create(THIS_MODULE, "mic");
	if(IS_ERR(mic_class))
	{
		ret = PTR_ERR(mic_class);
		printk(KERN_ALERT"Failed to create myfake class. \n");
		goto destroy_cdev;
	}
	// create device file myfake on /dev/  and /sys/class/myfake
	temp = device_create(mic_class, NULL, dev, "%s", "mic");
	if(IS_ERR(temp))
	{
		ret = PTR_ERR(temp);
		printk(KERN_ALERT"Failed to create myfake device.\n");
		goto destroy_class;
	} 


	//创建devfs文件系统接口设备
	// create property file val on /sys/class/myfake/myfake
	ret = device_create_file(temp, &dev_attr_val);
	if(ret < 0)
	{
		printk(KERN_ALERT"Failed to create attribute val.\n");
		goto destroy_device;
	}

	misc_setup_proc();
	dev_set_drvdata(temp, mic_dev);
    
    return ret;

destroy_device:
	device_destroy(mic_class, dev);
destroy_class:
	class_destroy(mic_class);
destroy_cdev:
	cdev_del(&mic_dev->dev);
cleanup:
	kfree(mic_dev);
unregister:
	unregister_chrdev_region(MKDEV(mic_major, mic_minor), 1);
//fail1:
//    platform_device_unregister(&mic_dev);
fail0:
    return ret;
}
module_init(mic_init);

static void __exit mic_exit(void)
{
	dev_t devno = MKDEV(mic_major, mic_minor);
	printk(KERN_ALERT"Destroy myfake device. \n");

	//删除devfs文件系统接口设备
	if(mic_class)
	{
		device_destroy(mic_class, MKDEV(mic_major, mic_minor) );
		class_destroy(mic_class);
	}
	//传统设备文件系统的接口设备
	if(mic_dev)
	{
		cdev_del(&(mic_dev ->dev) );
		kfree(mic_dev);
	}
	// destroy device number
	unregister_chrdev_region(devno, 1);
	remove_proc_entry(MISC_ENTRY_NAME, misc_dir);
	remove_proc_entry(MISC_DIR_NAME, NULL);
//    platform_driver_unregister(&mic_dev);
//    platform_device_unregister(&mic_dev);
}
module_exit(mic_exit);


MODULE_LICENSE(ATC_KERNEL_LINUX_LICENSE);
MODULE_AUTHOR("LMK");


