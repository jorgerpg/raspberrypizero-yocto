# Packages and users
IMAGE_INSTALL:append = " resize-rootfs "
IMAGE_INSTALL:append = " networkmanager-nmcli networkmanager-wifi dnsmasq avahi-daemon net-tools sudo chrony tzdata locale-base-en-us "
IMAGE_INSTALL:append = " python3-smbus i2c-tools "
IMAGE_INSTALL:append = " nano "
IMAGE_FEATURES:append = " ssh-server-openssh "

# Herda o recurso extrausers para criar usuarios
inherit extrausers

# Cria o usuario zero com senha "zero" (hash SHA-256)
# Adiciona aos grupos: sudo, video, audio, dialout, docker
EXTRA_USERS_PARAMS = " \
    useradd -m -s /bin/sh -p '\$5\$XoFz0robdXtzDW5f\$6.k3zMHu9VEB8BY1vfAf1qIScI064qdsBRte/aL3gZ6' zero; \
    usermod -aG sudo,video,audio,dialout,docker zero; \
"

# Permite que o grupo sudo execute qualquer comando
ROOTFS_POSTPROCESS_COMMAND:append = " sudo_access; "

sudo_access() {
    echo '%sudo ALL=(ALL) ALL' >> ${IMAGE_ROOTFS}${sysconfdir}/sudoers
}
