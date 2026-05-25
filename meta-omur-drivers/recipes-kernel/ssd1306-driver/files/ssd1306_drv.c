// SPDX-License-Identifier: GPL-2.0
/*
 * ssd1306_drv.c - SSD1306 OLED I2C Kernel Driver
 *
 * Omur Ceran - Kernel Driver Development Project
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "ssd1306"
#define CLASS_NAME  "oled"

/* SSD1306 commands */
#define SSD1306_DISPLAY_OFF     0xAE
#define SSD1306_DISPLAY_ON      0xAF
#define SSD1306_SET_CONTRAST    0x81
#define SSD1306_NORMAL_DISPLAY  0xA6
#define SSD1306_INVERT_DISPLAY  0xA7
#define SSD1306_SET_MUX_RATIO   0xA8
#define SSD1306_SET_DISP_OFFSET 0xD3
#define SSD1306_SET_START_LINE  0x40
#define SSD1306_CHARGE_PUMP     0x8D
#define SSD1306_MEMORY_MODE     0x20
#define SSD1306_SEG_REMAP       0xA1
#define SSD1306_COM_SCAN_DEC    0xC8
#define SSD1306_SET_COM_PINS    0xDA
#define SSD1306_SET_PRECHARGE   0xD9
#define SSD1306_SET_VCOMH       0xDB
#define SSD1306_SET_CLK_DIV     0xD5
#define SSD1306_ENTIRE_ON_RES   0xA4

/* Display parameters */
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8)
#define SSD1306_BUFSIZE (SSD1306_WIDTH * SSD1306_PAGES)

struct ssd1306_data {
    struct i2c_client *client;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    dev_t devnum;
    u8 buffer[SSD1306_BUFSIZE];
};

/* Send command byte to SSD1306 */
static int ssd1306_write_cmd(struct i2c_client *client, u8 cmd)
{
    /* Control byte: Co=0, D/C#=0 (command) */
    return i2c_smbus_write_byte_data(client, 0x00, cmd);
}

/* Send data byte to SSD1306 */
static int ssd1306_write_data(struct i2c_client *client, u8 data)
{
    /* Control byte: Co=0, D/C#=1 (data) */
    return i2c_smbus_write_byte_data(client, 0x40, data);
}

/* Initialize SSD1306 display */
static int ssd1306_init_display(struct i2c_client *client)
{
    int ret;

    dev_info(&client->dev, "Initializing SSD1306 display\n");

    ret = ssd1306_write_cmd(client, SSD1306_DISPLAY_OFF);
    if (ret < 0) return ret;

    ssd1306_write_cmd(client, SSD1306_SET_CLK_DIV);
    ssd1306_write_cmd(client, 0x80);

    ssd1306_write_cmd(client, SSD1306_SET_MUX_RATIO);
    ssd1306_write_cmd(client, 0x3F);  /* 64 lines */

    ssd1306_write_cmd(client, SSD1306_SET_DISP_OFFSET);
    ssd1306_write_cmd(client, 0x00);

    ssd1306_write_cmd(client, SSD1306_SET_START_LINE | 0x00);
    
    ssd1306_write_cmd(client, SSD1306_CHARGE_PUMP);
    ssd1306_write_cmd(client, 0x14);  /* Enable charge pump */

    ssd1306_write_cmd(client, SSD1306_MEMORY_MODE);
    ssd1306_write_cmd(client, 0x00);  /* Horizontal addressing */

    ssd1306_write_cmd(client, SSD1306_SEG_REMAP);       /* Segment remap */
    ssd1306_write_cmd(client, SSD1306_COM_SCAN_DEC);     /* COM scan dec */

    ssd1306_write_cmd(client, SSD1306_SET_COM_PINS);
    ssd1306_write_cmd(client, 0x12);

    ssd1306_write_cmd(client, SSD1306_SET_CONTRAST);
    ssd1306_write_cmd(client, 0xCF);

    ssd1306_write_cmd(client, SSD1306_SET_PRECHARGE);
    ssd1306_write_cmd(client, 0xF1);

    ssd1306_write_cmd(client, SSD1306_SET_VCOMH);
    ssd1306_write_cmd(client, 0x40);

    ssd1306_write_cmd(client, SSD1306_ENTIRE_ON_RES);
    ssd1306_write_cmd(client, SSD1306_NORMAL_DISPLAY);
    ssd1306_write_cmd(client, SSD1306_DISPLAY_ON);

    dev_info(&client->dev, "SSD1306 display initialized\n");
    return 0;
}

/* Clear display */
static void ssd1306_clear(struct ssd1306_data *drv)
{
    int i;

    memset(drv->buffer, 0x00, SSD1306_BUFSIZE);
    
    ssd1306_write_cmd(drv->client, 0x21); /* Column addr */
    ssd1306_write_cmd(drv->client, 0);
    ssd1306_write_cmd(drv->client, SSD1306_WIDTH - 1);
    ssd1306_write_cmd(drv->client, 0x22); /* Page addr */
    ssd1306_write_cmd(drv->client, 0);
    ssd1306_write_cmd(drv->client, SSD1306_PAGES - 1);

    for (i = 0; i < SSD1306_BUFSIZE; i++)
        ssd1306_write_data(drv->client, 0x00);
}

/* File operations - write: userspace'ten gelen veriyi ekrana bas */
static ssize_t ssd1306_fop_write(struct file *filp, const char __user *buf,
                                  size_t count, loff_t *offset)
{
    struct ssd1306_data *drv = filp->private_data;
    size_t len;
    int i;

    len = min(count, (size_t)SSD1306_BUFSIZE);

    if (copy_from_user(drv->buffer, buf, len))
        return -EFAULT;

    /* Set full window */
    ssd1306_write_cmd(drv->client, 0x21);
    ssd1306_write_cmd(drv->client, 0);
    ssd1306_write_cmd(drv->client, SSD1306_WIDTH - 1);
    ssd1306_write_cmd(drv->client, 0x22);
    ssd1306_write_cmd(drv->client, 0);
    ssd1306_write_cmd(drv->client, SSD1306_PAGES - 1);

    for (i = 0; i < len; i++)
        ssd1306_write_data(drv->client, drv->buffer[i]);

    return len;
}

static int ssd1306_fop_open(struct inode *inode, struct file *filp)
{
    struct ssd1306_data *drv;
    drv = container_of(inode->i_cdev, struct ssd1306_data, cdev);
    filp->private_data = drv;
    return 0;
}

static int ssd1306_fop_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations ssd1306_fops = {
    .owner   = THIS_MODULE,
    .open    = ssd1306_fop_open,
    .release = ssd1306_fop_release,
    .write   = ssd1306_fop_write,
};

/* I2C probe - kernel cihazı bulduğunda çağrılır */
static int ssd1306_probe(struct i2c_client *client)
{
    struct ssd1306_data *drv;
    int ret;

    dev_info(&client->dev, "SSD1306 probe called, addr=0x%02x\n", client->addr);

    drv = devm_kzalloc(&client->dev, sizeof(*drv), GFP_KERNEL);
    if (!drv)
        return -ENOMEM;

    drv->client = client;
    i2c_set_clientdata(client, drv);

    /* Character device oluştur: /dev/oled0 */
    ret = alloc_chrdev_region(&drv->devnum, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        dev_err(&client->dev, "alloc_chrdev_region failed\n");
        return ret;
    }

    cdev_init(&drv->cdev, &ssd1306_fops);
    drv->cdev.owner = THIS_MODULE;
    ret = cdev_add(&drv->cdev, drv->devnum, 1);
    if (ret < 0) {
        dev_err(&client->dev, "cdev_add failed\n");
        goto err_cdev;
    }

    drv->class = class_create(CLASS_NAME);
    if (IS_ERR(drv->class)) {
        ret = PTR_ERR(drv->class);
        goto err_class;
    }

    drv->device = device_create(drv->class, NULL, drv->devnum, NULL, "oled0");
    if (IS_ERR(drv->device)) {
        ret = PTR_ERR(drv->device);
        goto err_device;
    }

    /* Display'i başlat ve temizle */
    ret = ssd1306_init_display(client);
    if (ret < 0) {
        dev_err(&client->dev, "Display init failed\n");
        goto err_init;
    }

    ssd1306_clear(drv);
    dev_info(&client->dev, "SSD1306 driver loaded -> /dev/oled0\n");
    return 0;

err_init:
    device_destroy(drv->class, drv->devnum);
err_device:
    class_destroy(drv->class);
err_class:
    cdev_del(&drv->cdev);
err_cdev:
    unregister_chrdev_region(drv->devnum, 1);
    return ret;
}

/* I2C remove - cihaz çıkarıldığında veya rmmod yapıldığında */
static void ssd1306_remove(struct i2c_client *client)
{
    struct ssd1306_data *drv = i2c_get_clientdata(client);

    ssd1306_write_cmd(client, SSD1306_DISPLAY_OFF);

    device_destroy(drv->class, drv->devnum);
    class_destroy(drv->class);
    cdev_del(&drv->cdev);
    unregister_chrdev_region(drv->devnum, 1);

    dev_info(&client->dev, "SSD1306 driver removed\n");
}

/* Device tree eşleşmesi */
static const struct of_device_id ssd1306_of_match[] = {
    { .compatible = "omur,ssd1306" },
    { }
};
MODULE_DEVICE_TABLE(of, ssd1306_of_match);

/* I2C device ID tablosu */
static const struct i2c_device_id ssd1306_id[] = {
    { "ssd1306", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ssd1306_id);

/* I2C driver yapısı */
static struct i2c_driver ssd1306_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = ssd1306_of_match,
    },
    .probe = ssd1306_probe,
    .remove = ssd1306_remove,
    .id_table = ssd1306_id,
};

module_i2c_driver(ssd1306_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Omur Ceran");
MODULE_DESCRIPTION("SSD1306 OLED I2C Character Device Driver");
MODULE_VERSION("1.0");