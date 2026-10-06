# Network, DNS, timezone and locale setup
ROOTFS_POSTPROCESS_COMMAND:append = " set_hostname; set_hosts; tweak_avahi; setup_networkmanager; set_resolv_conf; setup_chrony; setup_timezone; setup_locale; "

# DNS fixo, aplicado pelo NetworkManager a todas as conexoes
DNS = "8.8.8.8 8.8.4.4 1.1.1.1"

setup_networkmanager() {
    install -d ${IMAGE_ROOTFS}${sysconfdir}/NetworkManager
    cat > ${IMAGE_ROOTFS}${sysconfdir}/NetworkManager/NetworkManager.conf << 'EOF'
[main]
plugins=keyfile
rc-manager=file

[global-dns-domain-*]
servers=${@','.join(d.getVar('DNS').split())}

[device]
wifi.scan-rand-mac-address=no

[logging]
level=INFO
EOF
}

set_hostname() {
    echo "zero" > ${IMAGE_ROOTFS}${sysconfdir}/hostname
}

set_hosts() {
    cat > ${IMAGE_ROOTFS}${sysconfdir}/hosts << 'EOF'
127.0.0.1       localhost
127.0.1.1       zero

::1             localhost ip6-localhost ip6-loopback
ff02::1         ip6-allnodes
ff02::2         ip6-allrouters
EOF
}

tweak_avahi() {
    if [ -f ${IMAGE_ROOTFS}${sysconfdir}/avahi/avahi-daemon.conf ]; then
        sed -i 's/^[# ]*publish-workstation=.*/publish-workstation=yes/' \
            ${IMAGE_ROOTFS}${sysconfdir}/avahi/avahi-daemon.conf || true
    fi
}

# Arquivo regular valido ate o NetworkManager subir e reescreve-lo (rc-manager=file)
set_resolv_conf() {
    rm -f ${IMAGE_ROOTFS}${sysconfdir}/resolv.conf
    for ns in ${DNS}; do
        echo "nameserver $ns"
    done > ${IMAGE_ROOTFS}${sysconfdir}/resolv.conf
}

setup_timezone() {
    echo 'America/Sao_Paulo' > ${IMAGE_ROOTFS}${sysconfdir}/timezone
    rm -f ${IMAGE_ROOTFS}${sysconfdir}/localtime
    ln -sf /usr/share/zoneinfo/America/Sao_Paulo ${IMAGE_ROOTFS}${sysconfdir}/localtime
}

setup_chrony() {
    cat > ${IMAGE_ROOTFS}${sysconfdir}/chrony.conf << 'EOF'
server 0.pool.ntp.org iburst
server 1.pool.ntp.org iburst
server 2.pool.ntp.org iburst
server 3.pool.ntp.org iburst

makestep 1.0 3

driftfile /var/lib/chrony/drift
rtcsync
logdir /var/log/chrony
EOF
}

setup_locale() {
    cat > ${IMAGE_ROOTFS}${sysconfdir}/locale.conf << 'EOF'
LANG=en_US.UTF-8
LC_ALL=en_US.UTF-8
EOF
    cat >> ${IMAGE_ROOTFS}${sysconfdir}/profile << 'EOF'
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
EOF
}
