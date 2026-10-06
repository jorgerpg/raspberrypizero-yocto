# NetworkManager gerencia a rede e o /etc/resolv.conf, e o chrony cuida do NTP.
# Sem isso, systemd-networkd/resolved/timesyncd ficam ativos e competem com eles.
PACKAGECONFIG:remove = "networkd resolved nss-resolve timesyncd"
