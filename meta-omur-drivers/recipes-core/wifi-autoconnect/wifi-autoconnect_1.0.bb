SUMMARY = "WiFi Auto Connect Service"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://wpa_supplicant-wlan0.conf \
    file://wifi-autoconnect.service \
    file://wifi-dhcp.service \
"

inherit systemd

SYSTEMD_SERVICE:${PN} = "wifi-autoconnect.service wifi-dhcp.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install() {
    install -d ${D}${sysconfdir}
    install -m 0600 ${WORKDIR}/wpa_supplicant-wlan0.conf ${D}${sysconfdir}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/wifi-autoconnect.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/wifi-dhcp.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} = " \
    ${sysconfdir}/wpa_supplicant-wlan0.conf \
    ${systemd_system_unitdir}/wifi-autoconnect.service \
    ${systemd_system_unitdir}/wifi-dhcp.service \
"

RDEPENDS:${PN} = "wpa-supplicant"