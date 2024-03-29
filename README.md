# LCD_HD44780_driver

## Discripton
Driver for Raspbery PI 3/4 to dispaly string on LCD screen with
controller HD44780. It implemented such as file in /proc.
**This code will work with Linux kernel version > 6.1.**

Find out the version number:
```sh
uname -a
```

If you start on PI 4:
```C
gpio_registers = (int*)ioremap(BCM2711_GPIO_ADDRESS, PAGE_SIZE);
```

If you start on PI 3:
```C
gpio_registers = (int*)ioremap(BCM2837_GPIO_ADDRESS, PAGE_SIZE);
```

## How work with it
If you want use it, run:

```sh
sudo insmod HD44780_driver.ko
```

Then, if you want to show string in display, run:

```sh
echo "Hello 5" > /proc/HD44780_driver
```

if you want to customize it:

```sh
sudo apt install raspberrypi-kernel-headers
```

When you finished change code, compile it with:

```sh
make
```

To unset driver from kernel:
```sh
sudo rmmod HD44780_driver
```

To set driver in kernel:

```sh
sudo insmod HD44780_driver.ko
```

To show messages from driver:

```sh
dmesg
```

### Connection of display:
![](https://github.com/alexmangushev/Raspberry_LCD_HD44780_driver/blob/master/img/schem.png)


