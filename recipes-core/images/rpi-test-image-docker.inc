# Docker service and permissions
ROOTFS_POSTPROCESS_COMMAND:append = " setup_docker_socket_override; setup_docker_systemd_override; ensure_docker_group; enable_docker_service; setup_docker_recover_service; "

enable_docker_service() {
    install -d ${IMAGE_ROOTFS}${sysconfdir}/systemd/system
    cat > ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/docker.service << 'EOF'
[Unit]
Description=Docker Engine (custom)
After=network.target local-fs.target

[Service]
Type=simple
ExecStartPre=/bin/sh -c 'mkdir -p /var/run/docker/containerd /var/lib/docker/containerd/daemon || true'
ExecStartPre=/bin/sh -c 'rm -f /var/run/docker.sock /var/run/docker/containerd/containerd.sock* /var/run/docker/containerd/*.pid || true'
ExecStart=/usr/bin/dockerd -G docker
Restart=always
RestartSec=3
TimeoutStartSec=300
TimeoutStopSec=120

[Install]
WantedBy=multi-user.target
EOF

    install -d ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/multi-user.target.wants
    ln -sf ${sysconfdir}/systemd/system/docker.service \
        ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/multi-user.target.wants/docker.service

    ln -sf /dev/null ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/docker.socket
    rm -f ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/sockets.target.wants/docker.socket
}

setup_docker_recover_service() {
    install -d ${IMAGE_ROOTFS}/usr/bin
    cat > ${IMAGE_ROOTFS}/usr/bin/docker-recover.sh << 'EOF'
#!/bin/sh
tries=0
while [ "$tries" -lt 30 ]; do
    if docker info >/dev/null 2>&1; then
        exit 0
    fi

    systemctl start docker.service >/dev/null 2>&1 || true
    sleep 2
    tries=$((tries + 1))
done

exit 1
EOF
    chmod 0755 ${IMAGE_ROOTFS}/usr/bin/docker-recover.sh

    install -d ${IMAGE_ROOTFS}${systemd_unitdir}/system
    cat > ${IMAGE_ROOTFS}${systemd_unitdir}/system/docker-recover.service << 'EOF'
[Unit]
Description=Ensure Docker daemon is ready
After=network.target docker.service
Wants=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/bin/docker-recover.sh

[Install]
WantedBy=multi-user.target
EOF

    install -d ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/multi-user.target.wants
    ln -sf ${systemd_unitdir}/system/docker-recover.service \
        ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/multi-user.target.wants/docker-recover.service
}

setup_docker_socket_override() {
    install -d ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/docker.socket.d
    cat > ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/docker.socket.d/override.conf << 'EOF'
[Socket]
SocketGroup=docker
SocketMode=0660
EOF
}

setup_docker_systemd_override() {
    install -d ${IMAGE_ROOTFS}/usr/bin
    cat > ${IMAGE_ROOTFS}/usr/bin/fix-docker-sock-perms.sh << 'EOF'
#!/bin/sh
for _ in 1 2 3 4 5 6 7 8 9 10; do
    if [ -S /var/run/docker.sock ]; then
        chgrp docker /var/run/docker.sock 2>/dev/null || true
        chmod 0660 /var/run/docker.sock 2>/dev/null || true
        exit 0
    fi
    sleep 1
done
exit 0
EOF
    chmod 0755 ${IMAGE_ROOTFS}/usr/bin/fix-docker-sock-perms.sh

    install -d ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/docker.service.d
    cat > ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/docker.service.d/override.conf << 'EOF'
[Service]
ExecStartPost=/usr/bin/fix-docker-sock-perms.sh
EOF
}

ensure_docker_group() {
    if grep -q '^docker:' ${IMAGE_ROOTFS}${sysconfdir}/group; then
        if ! grep '^docker:' ${IMAGE_ROOTFS}${sysconfdir}/group | grep -q '\bzero\b'; then
            sed -i -E 's/^(docker:[^:]*:[0-9]*:)(.*)$/\1\2,zero/' ${IMAGE_ROOTFS}${sysconfdir}/group
            sed -i 's/:,zero$/:zero/' ${IMAGE_ROOTFS}${sysconfdir}/group
        fi
    fi
}
