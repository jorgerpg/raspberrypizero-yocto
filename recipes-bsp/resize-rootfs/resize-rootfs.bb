SUMMARY = "Grow the /data partition and filesystem on first boot (systemd)"
LICENSE = "CLOSED"

inherit systemd

SRC_URI = " \
    file://resize-rootfs.sh \
    file://resize-rootfs.service \
"

S = "${WORKDIR}"

SYSTEMD_SERVICE:${PN} = "resize-rootfs.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

RDEPENDS:${PN} = "e2fsprogs-resize2fs util-linux-findmnt util-linux-sfdisk util-linux-partx"

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/resize-rootfs.sh ${D}${sbindir}/resize-rootfs.sh

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/resize-rootfs.service ${D}${systemd_system_unitdir}/resize-rootfs.service
}
