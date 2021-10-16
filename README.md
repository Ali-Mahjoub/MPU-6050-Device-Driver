# MPU-6050-Device-Driver

![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/mpu6050.png)

<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#introduction">Introduction</a></li>  
    <li><a href="#walking-through-the-code">Walking through the code</a></li>
      <ul>
        <li><a href="#i2c-configuration-and-the-mpu6050-module-declaration">I2c configuration and the mpu6050 module declaration</a></li>
        <li><a href="#module-inititialisation-and-resetting">Module inititialisation and Resetting</a></li>
        <li><a href="#data-acquisition-functions">Data acquisition Functions</a></li>
        <li><a href="#file-operations">File operations</a></li>
      </ul>
    </li>      
    <li><a href="#conclusion">Conclusion</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgements">Acknowledgements</a></li>
       
  </ol>
</details>

## Introduction
* MPU6050 sensor module is an integrated 6-axis Motion tracking device.
* It has a 3-axis Gyroscope, 3-axis Accelerometer, Digital Motion Processor and a Temperature sensor, all in a single IC.
* A microcontroller can communicate with this module using I2C communication protocol. Various parameters can be found by reading values from addresses of certain registers using I2C communication.
* To interface MPU6050 module with Raspberry Pi using the i2c protocol i have written this C script to set the i2c configuration and to read telemetry data provided by this module and display it on the userspace's terminal.
## Walking through the code:
To write an I2C device driver in LINUX kernel there are several steps to follow:
1.Get the I2C adapter.
2.Create the info board structure and create a device using that.
3.Create the device id for your slave device and register that.
4.Create the i2c driver structure and add that to the I2C subsystem.
and then we can extchange data with our module
### I2c configuration and the mpu6050 module declaration
* **Instanciation of a new I2C device**
```c
static struct i2c_adapter * mpu6050_i2c_adapter = NULL;
static struct i2c_client * mpu6050_i2c_client = NULL;
```
* **Device Identification**
```c
#define mem_size        1024           //Memory Size
#define DRIVER_NAME "mpu6050"
#define DRIVER_CLASS "mpu6050Class"
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

```
### Module inititialisation and Resetting
* **Device variables and DeviceClass**
```c
static dev_t myDeviceNr;
static struct class *myClass;
static struct cdev myDevice;
char *data;
```
* **Init function**
```c
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
```
* **Exit function** 
```c
static void __exit ModuleExit(void) {
	printk("MyDeviceDriver - Goodbye, Kernel!\n");
	i2c_unregister_device(mpu6050_i2c_client);
	i2c_del_driver(&mpu6050_driver);
	cdev_del(&myDevice);
    device_destroy(myClass, myDeviceNr);
    class_destroy(myClass);
    unregister_chrdev_region(myDeviceNr, 1);
}
```
### Data acquisition Functions

This functions are used to copy the data from the module and write it into the kernel
Reffering to the MPU6050 Datasheet we map the data registers and teh calibration factors

* **Reading temperature data**

![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/temp_reg.PNG)
![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/temp_calibration.PNG)
```c
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
```
* **Reading acceleration data**


![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/acc_reg.PNG)
![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/acc_calibration.PNG)
```c
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
```
* **Reading gyroscope data**


![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/gyro_reg.PNG)
![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/gyro_calibration.PNG)
```c
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
```
### File operations
The file_operations structure is how a char driver sets up this connection. The structure, (defined in <linux/fs.h>), is a collection of function pointers. Each open file is associated with its own set of functions. The operations are mostly in charge of implementing the system calls and are, therefore, named open, read, and so on.
```c
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.read = driver_read,
};
```
* **Open file function**
<br/>This function is called first, whenever we are opening the device file. In this function, I am going to allocate the memory using kmalloc. In the userspace application, you can use open() system call to open the device file.<br/>
```c
static int driver_open(struct inode *deviceFile, struct file *instance) {
if((data = kmalloc(mem_size , GFP_KERNEL)) == 0){
            pr_info("Cannot allocate memory in kernel\n");
            return -1;
        }
	printk("MyDeviceDriver -  Open was called\n");
	return 0;
}

```
* **Close File function**
<br>When we close the device file that will call this function. Here I will free the memory that is allocated by kmalloc using kfree(). In the userspace application, you can use close() system call to close the device file.<br/>
```c
static int driver_close(struct inode *deviceFile, struct file *instance) {
	kfree(data);
	printk("MyDeviceDriver -  Close was called\n");
	return 0;
}

```
* **Writing data in the userspace**
<br>When we read the device file it will call this function. In this function, I used copy_to_user(). This function is used to copy the data to the userspace application. In the userspace application, you can use read() system call to read the data from the device file.<br/>
```c
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
```

## Test benchmark
![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/mpu6050_test.jpg)
![results_1](https://github.com/Ali-Mahjoub/MPU-6050-Device-Driver/blob/main/images/results1.PNG)

## Conclusion:
i impelemented through this snippet of code a simple I2C MPU6050 device driver that could be used to display the sensor data with our own developed functions . 
  
### Contact:
* Mail : ali.mahjoub1998@gmail.com 
* Linked-in profile: https://www.linkedin.com/in/ali-mahjoub-b83a86196/

### Acknowledgements:
* The Embetronicx admins: https://embetronicx.com/
