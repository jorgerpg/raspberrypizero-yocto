# connman vem daqui e conflita com o NetworkManager (RCONFLICTS no networkmanager-daemon)
RDEPENDS:${PN}:remove = "connman connman-client"
