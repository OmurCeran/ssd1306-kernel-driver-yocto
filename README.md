# SSD1306 OLED Kernel Driver - Yocto BSP Layer

Custom Linux kernel driver for SSD1306 OLED display (128x64, I2C) with Yocto Scarthgap integration on Raspberry Pi 3.

## Architecture

```
┌─────────────────────────────────────────────────┐
│  Userspace                                      │
│  ┌──────────┐    write()    ┌───────────────┐   │
│  │ oled-menu ├─────────────►│  /dev/oled0   │   │
│  └──────────┘               └───────┬───────┘   │
├─────────────────────────────────────┼───────────┤
│  Kernel                            │            │
│  ┌──────────────────────────────────▼─────────┐ │
│  │  ssd1306_drv.ko                            │ │
│  │  - I2C client driver                       │ │
│  │  - Character device (/dev/oled0)           │ │
│  │  - SSD1306 init sequence                   │ │
│  │  - Framebuffer write via file_operations   │ │
│  └──────────────────────────┬─────────────────┘ │
│                             │ i2c_smbus_*       │
│  ┌──────────────────────────▼─────────────────┐ │
│  │  i2c-bcm2835 (I2C bus driver)              │ │
│  └──────────────────────────┬─────────────────┘ │
├─────────────────────────────┼───────────────────┤
│  Hardware                   │                   │
│  RPi3 I2C1 ────── SDA/SCL ─┴── SSD1306 @ 0x3C  │
└─────────────────────────────────────────────────┘
```

## Features

- **Out-of-tree kernel module** (`ssd1306_drv.ko`) with proper I2C probe/remove lifecycle
- **Character device** (`/dev/oled0`) for userspace framebuffer access
- **Device Tree** overlay for automatic device registration
- **Systemd service** for auto-loading driver at boot
- **Userspace menu application** with 5x7 bitmap font rendering
- **WiFi auto-connect** service with wpa_supplicant
- **Yocto Scarthgap 5.0 LTS** (kernel 6.6) on Raspberry Pi 3 (aarch64)

## Layer Structure

```
meta-omur-drivers/
├── conf/
│   └── layer.conf
├── recipes-kernel/
│   └── ssd1306-driver/
│       ├── files/
│       │   ├── ssd1306_drv.c        # Kernel module source
│       │   └── Makefile              # Kbuild makefile
│       └── ssd1306-driver_1.0.bb     # Module recipe
├── recipes-core/
│   ├── ssd1306-autoload/
│   │   ├── files/
│   │   │   └── ssd1306-autoload.service
│   │   └── ssd1306-autoload_1.0.bb
│   └── wifi-autoconnect/
│       ├── files/
│       │   ├── wpa_supplicant-wlan0.conf
│       │   ├── wifi-autoconnect.service
│       │   └── wifi-dhcp.service
│       └── wifi-autoconnect_1.0.bb
└── recipes-apps/
    └── oled-menu/
        ├── files/
        │   ├── oled_menu.c           # Menu application
        │   └── CMakeLists.txt
        └── oled-menu_1.0.bb
```

## Hardware Setup

| SSD1306 Pin | RPi3 Pin | GPIO    |
|-------------|----------|---------|
| VCC         | Pin 1    | 3.3V    |
| GND         | Pin 6    | GND     |
| SDA         | Pin 3    | GPIO2   |
| SCL         | Pin 5    | GPIO3   |

## Build Instructions

### Prerequisites

- Ubuntu 22.04 / 24.04 (or WSL2)
- Yocto build dependencies installed
- ~100GB free disk space

### Setup

```bash
# Clone Yocto layers
mkdir -p ~/yocto-rpi && cd ~/yocto-rpi
git clone -b scarthgap https://git.yoctoproject.org/poky
git clone -b scarthgap https://git.yoctoproject.org/meta-raspberrypi
git clone -b scarthgap https://github.com/openembedded/meta-openembedded.git

# Clone this layer
git clone https://github.com/OmurCeran/ssd1306-yocto-driver.git meta-omur-drivers

# Initialize build
cd poky
source oe-init-build-env build-rpi3
```

### Configure

Add to `conf/bblayers.conf`:
```
BBLAYERS += "/path/to/meta-omur-drivers"
```

Add to `conf/local.conf`:
```
MACHINE = "raspberrypi3-64"
RPI_USE_U_BOOT = "1"
ENABLE_I2C = "1"
ENABLE_UART = "1"
INIT_MANAGER = "systemd"
KERNEL_MODULE_AUTOLOAD:append = " i2c-dev i2c-bcm2835 ssd1306_drv"
IMAGE_INSTALL:append = " ssd1306-driver ssd1306-autoload oled-menu"
IMAGE_INSTALL:append = " i2c-tools openssh-sftp-server openssh"
```

### Build

```bash
bitbake core-image-minimal
```

### Flash

```bash
bzip2 -dk tmp/deploy/images/raspberrypi3-64/core-image-minimal-raspberrypi3-64.wic.bz2
# Flash with Balena Etcher or dd
```

## Usage

```bash
# Verify I2C device
i2cdetect -y 1      # Should show 0x3c

# Check driver
dmesg | grep ssd1306
ls -la /dev/oled0    # Should show crw-------

# Run menu application
oled-menu
# Controls: w=up, s=down, q=quit

# Manual framebuffer test
dd if=/dev/urandom bs=1024 count=1 > /dev/oled0   # Random pixels
dd if=/dev/zero bs=1024 count=1 > /dev/oled0       # Clear screen
```

## Cross-Compile with SDK

```bash
# Build and install SDK
bitbake core-image-minimal -c populate_sdk

# Activate SDK
source /opt/poky/5.0.18/environment-setup-cortexa53-poky-linux

# Compile application
$CC oled_menu.c -o oled-menu

# Deploy to target
scp oled-menu root@<RPI_IP>:/usr/bin/
```

## Technical Details

### Kernel Driver

The driver implements a standard Linux I2C client driver with character device interface:

- **probe()**: Called when device tree match or `new_device` triggers. Initializes SSD1306 hardware, creates `/dev/oled0`.
- **remove()**: Display off, cleanup character device.
- **write()**: Accepts raw framebuffer data (1024 bytes = 128x8 pages) from userspace.
- Uses `devm_kzalloc` for automatic memory management tied to device lifecycle.
- Compatible string: `"omur,ssd1306"` for device tree matching.

### Menu Application

- Renders 5x7 bitmap font to 128x64 framebuffer
- Writes raw pixel data to `/dev/oled0`
- Interactive terminal menu with keyboard navigation

## License

- Kernel module: GPL-2.0
- Userspace application: MIT

## Author

Omur Ceran - Embedded Systems Engineer
