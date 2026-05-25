SUMMARY = "SSD1306 OLED I2C Kernel Driver"
DESCRIPTION = "Out-of-tree kernel module for SSD1306 OLED display over I2C"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://ssd1306_drv.c;beginline=1;endline=2;md5=5f1db1a4251755b003fcaedee0489dba"

inherit module

SRC_URI = " \
    file://ssd1306_drv.c \
    file://Makefile \
"

S = "${WORKDIR}"

RPROVIDES:${PN} += "kernel-module-ssd1306-drv"