SUMMARY = "Auto-load SSD1306 I2C device"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ssd1306-autoload.service"

inherit systemd

SYSTEMD_SERVICE:${PN} = "ssd1306-autoload.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/ssd1306-autoload.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} = "${systemd_system_unitdir}/ssd1306-autoload.service"