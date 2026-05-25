SUMMARY = "OLED Dashboard Menu Application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://oled_menu.c \
    file://CMakeLists.txt \
"

S = "${WORKDIR}"

inherit cmake

FILES:${PN} = "${bindir}/oled-menu"