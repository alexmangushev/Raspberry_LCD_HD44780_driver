#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/slab.h>

#include <asm/io.h>

#define MAX_USER_SIZE 32

#define BCM2837_GPIO_ADDRESS 0x3F200000
#define BCM2711_GPIO_ADDRESS 0xFE200000

// GPIO to LCD mapping
// Pi pins
#define LCD_RS 21 
#define LCD_RW 26 
#define LCD_E  20 
#define LCD_D4 16 
#define LCD_D5 19 
#define LCD_D6 13 
#define LCD_D7 12

// Device constants
#define LCD_CHR 1     // Character mode
#define LCD_CMD 0     // Command mode

// arrays for Initilizetion pins
static uint8_t pins_out_1[] = {12, 13, 16, 19};
static uint8_t pins_out_1_size = 4;
static uint8_t pins_out_2[] = {20, 21, 22, 23, 24, 26, 27};
static uint8_t pins_out_2_size = 7;


static struct proc_dir_entry *lll_proc = NULL;
static char data_buffer[LLL_MAX_USER_SIZE+1] = {0};
static volatile uint32_t *gpio_registers = NULL;

// Set or clear GPIO pin
// \param [in] pin   - number of pin
// \param [in] state - state of pin
static void gpio_pin_set(uint8_t pin, uint8_t state)
{
	// get adresses of base registers for set and clear pins
	unsigned int* gpset_0 = (uint32_t *)((char*)gpio_registers + 0x1c);
	unsigned int *gpclr_0 = (uint32_t *)((char*)gpio_registers + 0x28);
	
	if (state == 0)
	{
		__sync_synchronize();
		*gpclr_0 = (1 << pin);
		__sync_synchronize();
	}
	else
	{
		__sync_synchronize();
		*gpset_0 = (1 << pin);
		__sync_synchronize();
	}
		
	return;
}

// Initilizetion GPIO
static void gpio_pin_init(void)
{
	// get adresses of base registers for clear pins
	unsigned int *gpclr_0 = (unsigned int*)((char*)gpio_registers + 0x28);

	// addres of register for set direction of pin
	unsigned int* gpio_fsel = (unsigned int*)((char*)gpio_registers + 0x04);
	
	unsigned int gpio_fsel_mask = 0;
	uint8_t i = 0;
	for (i = 0; i < pins_out_1_size; i++)
	{
		gpio_fsel_mask |= (1 << ((pins_out_1[i] % 10)*3));
	}
	
	*gpio_fsel = 0;
	*gpio_fsel = gpio_fsel_mask;
	
	unsigned int* gpio_fsel_2 = (unsigned int*)((char*)gpio_registers + 0x08);
	gpio_fsel_mask = 0;
	for (i = 0; i < pins_out_2_size; i++)
	{
		gpio_fsel_mask |= (1 << ((pins_out_2[i] % 10)*3));
	}
	
	*gpio_fsel_2 = 0;
	*gpio_fsel_2 = gpio_fsel_mask;
	
	// clear all pins
	*gpclr_0 = ~((unsigned int)0);
	return;
}

// Toggle LCD_E pin to write data in displays
static void lcd_toggle_enable(void)
{
    udelay(500);
    gpio_pin_set(LCD_E, 1);
    udelay(500);
    gpio_pin_set(LCD_E, 0);
    udelay(500);
    return;
}

// Write data in display
// \param [in] bits - writing data
// \param [in] mode - command or char for display
static void lcd_write(uint8_t bits, uint8_t mode)
{
	gpio_pin_set(LCD_RS, mode); // RS

    // High bits
    gpio_pin_set(LCD_D4, 0);
    gpio_pin_set(LCD_D5, 0);
    gpio_pin_set(LCD_D6, 0);
    gpio_pin_set(LCD_D7, 0);
    
    if (bits & (1 << 4)) {gpio_pin_set(LCD_D4, 1);}
    if (bits & (1 << 5)) {gpio_pin_set(LCD_D5, 1);}
    if (bits & (1 << 6)) {gpio_pin_set(LCD_D6, 1);}     
    if (bits & (1 << 7)) {gpio_pin_set(LCD_D7, 1);}
 
    // Toggle 'Enable' pin
	lcd_toggle_enable();
	
    // Low bits
    gpio_pin_set(LCD_D4, 0);
    gpio_pin_set(LCD_D5, 0);
    gpio_pin_set(LCD_D6, 0);
    gpio_pin_set(LCD_D7, 0);
    
    
    if (bits & (1 << 0)) {gpio_pin_set(LCD_D4, 1);}       
    if (bits & (1 << 1)) {gpio_pin_set(LCD_D5, 1);}     
    if (bits & (1 << 2)) {gpio_pin_set(LCD_D6, 1);}       
    if (bits & (1 << 3)) {gpio_pin_set(LCD_D7, 1);}
  
    // Toggle 'Enable' pin
    lcd_toggle_enable();
	return;
}

// Initilizetion display 
static void lcd_init(void)
{
    lcd_write(0x33, LCD_CMD); // Initialize
    lcd_write(0x32, LCD_CMD); // Set to 4-bit mode
    lcd_write(0x06, LCD_CMD); // Cursor move direction
    lcd_write(0x0C, LCD_CMD); // Turn cursor off
    lcd_write(0x06, LCD_CMD); // 2 line display
    lcd_write(0x01, LCD_CMD); // Clear display
    udelay(500); // Delay to allow commands to process
	return;
}

// Write string in display
// \param [in] message - pointer to string
// \param [in] line    - size of string
static void lcd_text(char message[], uint8_t line)
{
	uint8_t i = 0;
	for (i = 0; i < line; i++) {lcd_write(message[i], LCD_CHR);}
	return;
}


ssize_t driver_read(struct file *file, char __user *user, size_t size, loff_t *off)
{
	return copy_to_user(user,"Hello!\n", 7) ? 0 : 7;
}

ssize_t driver_write(struct file *file, const char __user *user, size_t size, loff_t *off)
{
	char str[LLL_MAX_USER_SIZE] = {};
	unsigned int len = 0;
	
	memset(data_buffer, 0x00, sizeof(data_buffer));
	if (size > MAX_USER_SIZE)
	{
		size = MAX_USER_SIZE;
	}
	
	if (copy_from_user(data_buffer, user, size))
		return 0;
	
	printk("Data buffer: %s\n", data_buffer);
	
	if (sscanf(data_buffer, "%16s%d", str, &len) != 2)
	{
		printk("Inproper data format submitted\n");
		return size;
	}
	
	if (len > strlen(str) || len > 16)
	{
		printk("Invalid lenght of string\n");
		return size;
	}
	
	gpio_pin_init();
	lcd_init();
	mdelay(500);
	lcd_text(str, len);
	return size;
}

static const struct proc_ops lll_proc_fops = 
{
	.proc_read = driver_read,
	.proc_write = driver_write,
};

static int __init gpio_driver_init(void)
{
	printk("LCD driver starts!!\n");
	
	gpio_registers = (int*)ioremap(BCM2711_GPIO_ADDRESS, PAGE_SIZE);
	if (gpio_registers == NULL)
	{
		printk("Failed to map GPIO memory to driver\n");
		return -1;
	}
	printk("Successfully mapped in GPIO memory\n");
	
	// create an entry in the proc-fs
	lll_proc = proc_create("HD44780_driver", 0666, NULL, &lll_proc_fops);
	if (lll_proc == NULL)
	{
		return -1;
	}

	return 0;
}

static void __exit gpio_driver_exit(void)
{
	printk("Leaving LCD driver!!\n");
	iounmap(gpio_registers);
	proc_remove(lll_proc);
	return;
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("alexmangushev");
MODULE_DESCRIPTION("Driver for LCD_HD44780_16_2");
MODULE_VERSION("0.9");
