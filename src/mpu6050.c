#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/i2c-dev.h>

#define mem_size        1024           //Memory Size
#define DRIVER_NAME "mpu6050"
#define DRIVER_CLASS "mpu6050Class"

static struct i2c_adapter * mpu6050_i2c_adapter = NULL;
static struct i2c_client * mpu6050_i2c_client = NULL;

/* Meta Information */
MODULE_AUTHOR("Ali Mahjoub");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A driver for reading out MPU6050 telemetry data");
MODULE_SUPPORTED_DEVICE("NONE");

/* Defines for device identification */ 
#define I2C_BUS_AVAILABLE	1		/* The I2C Bus available on the raspberry */
#define SLAVE_DEVICE_NAME	"MPU6050"	/* Device and Driver Name */
#define MPU6050_SLAVE_ADDRESS	0x76		/* MPU6050 I2C address */

static const struct i2c_device_id mpu6050_id[] = {
		{ SLAVE_DEVICE_NAME, 0 }, 
		{ }
};

static struct i2c_driver mpu6050_driver = {
	.driver = {
		.name = SLAVE_DEVICE_NAME,
		.owner = THIS_MODULE
	}
};

static struct i2c_board_info mpu6050_i2c_board_info = {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME, MPU6050_SLAVE_ADDRESS)
};

/* Variables for Device and Deviceclass*/

static dev_t myDeviceNr;
static struct class *myClass;
static struct cdev myDevice;
char *data;

int read_temperature(void) {
	int temp;
	int temp_data[2];
	int temperature;
	/* Read Temperature */
	temp_data[0] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x41);
	temp_data[1] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x42);
	temp = (temp_data[0]<<8 | temp_data[1]);
	temperature= (temp/340+36);

	return temperature;
}
int *read_accelerations(void) {
	
        int acc_data[6];
        int acc[3];
        int *a=acc;
        /* Read acceleration data*/
        acc_data[0] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x3B);
        acc_data[1] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x3C);
		acc_data[2] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x3D);
		acc_data[3] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x3E);
		acc_data[4] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x3F);
		acc_data[5] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x40);
		
		acc[0] = (int)((acc_data[0] << 8 | acc_data[1])/ 16384);
    	acc[1] = (int)((acc_data[2] << 8 | acc_data[3])/ 16384);
    	acc[2] = (int)((acc_data[4] << 8 | acc_data[5])/ 16384);
    	
		
	return a;
	}
	int *read_gyroscope(void) {

        int gyro_data[6];
        int gyro[3];
        /* Read Gyro data */
        int *a=gyro;
        gyro_data[0] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x43);
        gyro_data[1] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x44);
		gyro_data[2] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x45);
		gyro_data[3] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x46);
		gyro_data[4] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x47);
		gyro_data[5] = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0x48);
		
		gyro[0] = (int)((gyro_data[0] << 8 | gyro_data[1])/ 131);
    	gyro[1] = (int)((gyro_data[2] << 8 | gyro_data[3])/ 131);
    	gyro[2] = (int)((gyro_data[4] << 8 | gyro_data[5])/ 131);
    	
		
	return a;
	}
// Get data out of buffer
 
 static ssize_t driver_read(struct file *filp, char __user *user_buffer, size_t len, loff_t *off)
{   
	int temperature;
	int *acceleration;
	int *gyroscope;
	
	/* Get data */
	temperature = read_temperature();
	acceleration = read_accelerations();
	gyroscope = read_gyroscope();
	
	snprintf(data, sizeof(data), "Température:%d\nAcceleration_x:%d\nAcceleration_y: %d\nAcceleration_z: %d\nGyroscope_x: %d\nGyroscope_y: %d\nGyroscope_z: %d\n", temperature, acceleration[0], acceleration[1], acceleration[2], gyroscope[0], gyroscope[1], gyroscope[2]);

	/* Copy Data to user */
	 int a = copy_to_user(user_buffer, data, mem_size);

if ( a ){
        pr_err("Data Read : Err!\n");
        }
        pr_info("Data Read : Done!\n");
	return mem_size;
        
   }


 // This function is called, when the device file is opened
 
static int driver_open(struct inode *deviceFile, struct file *instance) {
if((data = kmalloc(mem_size , GFP_KERNEL)) == 0){
            pr_info("Cannot allocate memory in kernel\n");
            return -1;
        }
	printk("MyDeviceDriver -  Open was called\n");
	return 0;
}


 // This function is called, when the device file is close
 
static int driver_close(struct inode *deviceFile, struct file *instance) {
	kfree(data);
	printk("MyDeviceDriver -  Close was called\n");
	return 0;
}

/* Map the file operations */
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.read = driver_read,
};


// a function, which is called after loading module to kernel, do initialization there
 
static int __init ModuleInit(void) {
	int ret = -1;
	u8 id;
	printk("MyDeviceDriver - Hello Kernel\n");

	/* Allocate Device Nr */
	if ( alloc_chrdev_region(&myDeviceNr, 0, 1, DRIVER_NAME) < 0) {
		printk("Device Nr. could not be allocated!\n");
	}
	printk("MyDeviceDriver - Device Nr %d was registered\n", myDeviceNr);

	/* Create Device Class */
	if ((myClass = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
		printk("Device Class can not be created!\n");
		goto ClassError;
	}

	/* Create Device file */
	if (device_create(myClass, NULL, myDeviceNr, NULL, "mpu6050") == NULL) {
		printk("Can not create device file!\n");
		goto FileError;
	}

	/* Initialize Device file */
	cdev_init(&myDevice, &fops);

	/* register device to kernel */
	if (cdev_add(&myDevice, myDeviceNr, 1) == -1) {
		printk("Registering of device to kernel failed!\n");
		goto KernelError;
	}

	mpu6050_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);

	if(mpu6050_i2c_adapter != NULL) {
		mpu6050_i2c_client = i2c_new_client_device(mpu6050_i2c_adapter, &mpu6050_i2c_board_info);
		if(mpu6050_i2c_client != NULL) {
			if(i2c_add_driver(&mpu6050_driver) != -1) {
				ret = 0;
			}
			else
				printk("Can't add driver...\n");
		}
		i2c_put_adapter(mpu6050_i2c_adapter);
	}
	printk("MPU6050 Driver added!\n");

	/* Read Chip ID */
	id = i2c_smbus_read_byte_data(mpu6050_i2c_client, 0xD0);
	printk("ID: 0x%x\n", id);

	return ret;

KernelError:
	device_destroy(myClass, myDeviceNr);
FileError:
	class_destroy(myClass);
ClassError:
	unregister_chrdev(myDeviceNr, DRIVER_NAME);
	return (-1);
}


 // a function, which is called when removing module from kernelfree alocated resources
 
static void __exit ModuleExit(void) {
	printk("MyDeviceDriver - Goodbye, Kernel!\n");
	i2c_unregister_device(mpu6050_i2c_client);
	i2c_del_driver(&mpu6050_driver);
	cdev_del(&myDevice);
    device_destroy(myClass, myDeviceNr);
    class_destroy(myClass);
    unregister_chrdev_region(myDeviceNr, 1);
}

module_init(ModuleInit);
module_exit(ModuleExit);
