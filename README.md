# Meta Rasp (Yocto / Scarthgap)

Layer Yocto (`meta-custom`) para gerar a imagem para Raspberry Pi Zero com Docker (32-bit).

Este repositorio entrega:

- Templates de configuracao de build (`templates/bblayers.conf` e `templates/local.conf`)
- Script `source-project.sh`, que prepara o ambiente de build e aplica os templates
- Layer `meta-custom`, que customiza a imagem `rpi-test-image` (do `meta-raspberrypi`) via `.bbappend`: rede com NetworkManager, resize de rootfs, Docker e deploy automatico de containers

## Versoes utilizadas neste build

| Componente | Versao / referencia |
|---|---|
| Yocto Project | Scarthgap (5.0) |
| DISTRO / DISTRO_VERSION | `poky` 5.0.17 |
| BitBake | 2.8.1 |
| MACHINE | `raspberrypi0` |
| Kernel | `linux-raspberrypi` 6.6.63 |
| Docker (docker-moby) | 25.0.9 (`a926bec8fc91332410133b24f3e9e3f5add13b48`) |
| Docker Compose | v2.26.0 |

Camadas (branch `scarthgap`, commit no momento deste build):

| Layer | Commit |
|---|---|
| `poky` | `3a3d07f625aee7b87ea1026526b756447c357652` |
| `meta-openembedded` | `0f00f8b9a21950640da8c5707343e5540133f86e` |
| `meta-virtualization` | `c141b3cd60ad3a1bbab22b4884742d4c082beda6` |
| `meta-raspberrypi` | `6ca1f75017cc5d5acdb8bb05634c4bc01fa049fd` |

Ao atualizar qualquer layer (`git pull`), revise esta tabela e valide o build antes de assumir compatibilidade.

## 1) O que o layer implementa

| Area | Implementacao |
|---|---|
| Hardware | UART (`ENABLE_UART`), I2C (`i2c-dev` carregado no boot), PWM (`dtoverlay=pwm`) |
| Rede | NetworkManager + `nmcli`, hotspot WiFi em `wlan0`, Avahi/mDNS (`zero.local`) |
| DNS | DNS global do NetworkManager (`DNS`: `8.8.8.8`, `8.8.4.4`, `1.1.1.1`) |
| Tempo | `chrony` + timezone `America/Sao_Paulo` |
| Usuario | usuario `zero` com sudo e grupos `video`, `audio`, `dialout`, `docker` |
| SSH | OpenSSH |
| Rootfs | resize automatico no primeiro boot (`resize-rootfs`) |
| Containers | Docker |
| Pacotes | `package_deb` + `package-management` |
| Saida | `.wic.bz2` e `.img` bruto para o Raspberry Pi Imager |

## 2) Estrutura do repositorio

```text
meta-zero/
├── conf/layer.conf                      # colecao "meta-custom"
├── source-project.sh                    # prepara o build e copia os templates
├── templates/
│   ├── bblayers.conf
│   └── local.conf
├── recipes-bsp/
│   ├── overlays/                        # .dts proprios compilados para .dtbo
│   └── resize-rootfs/                   # resize da particao raiz no 1o boot
└── recipes-core/
    ├── images/
    │   ├── rpi-test-image.bbappend      # so faz require dos .inc abaixo
    │   ├── rpi-test-image-packages.inc
    │   ├── rpi-test-image-network.inc
    │   ├── rpi-test-image-docker.inc
    │   ├── rpi-test-image-selfcheck.inc
    │   ├── rpi-test-image-overlays.inc
    │   └── rpi-test-image-output.inc
    ├── packagegroups/packagegroup-rpi-test.bbappend
    └── systemd/systemd_%.bbappend
```

## 3) Pre-requisitos

- Host Linux com as dependencias de build do Yocto instaladas
- Espaco em disco recomendado: >= 50 GB
- Acesso de rede para clonar os layers
- Todos os layers na branch `scarthgap`

## 4) Build passo a passo

### 4.1 Montar o workspace

Todos os layers, este repositorio e o diretorio de build ficam lado a lado (o `bblayers.conf` usa caminhos `${TOPDIR}/../<layer>`):

```text
<workspace>/
├── poky/
├── meta-openembedded/
├── meta-virtualization/
├── meta-raspberrypi/
├── meta-zero/            # este repositorio
└── build/                # criado pelo source-project.sh
```

```bash
cd <workspace>
git clone -b scarthgap https://git.yoctoproject.org/poky
git clone -b scarthgap https://git.yoctoproject.org/meta-virtualization
git clone -b scarthgap https://git.openembedded.org/meta-openembedded
git clone -b scarthgap https://github.com/agherzan/meta-raspberrypi.git
git clone <url-deste-repo> meta-zero
```

### 4.2 Inicializar o ambiente

A partir do `<workspace>`:

```bash
source meta-zero/source-project.sh
```

O script roda o `oe-init-build-env` (diretorio `build/`, ou o definido em `$BUILDDIR`) e copia `templates/bblayers.conf` e `templates/local.conf` para `build/conf/`.

Atencao: a copia acontece toda vez que o script e executado, sobrescrevendo `build/conf/local.conf`. Alteracoes permanentes devem ser feitas em `templates/`.

### 4.3 Build da imagem

```bash
bitbake rpi-test-image
```

Artefatos em `tmp/deploy/images/raspberrypi0/`.

## 5) Detalhes do layer

### 5.1 Configuracao de build (`templates/local.conf`)

- `MACHINE = "raspberrypi0"`, distro `poky`
- systemd como init (`sysvinit` removido) e `usrmerge`
- `virtualization` em `DISTRO_FEATURES` (necessario para o Docker)
- `docker` e `docker-compose` instalados na imagem
- `ENABLE_UART`, `ENABLE_I2C` e autoload do modulo `i2c-dev`
- `RPI_EXTRA_CONFIG` com `dtparam=spi=on, enable_uart=1, dtparam=i2c_arm=on, dtoverlay=w5500,int_pin=12,reset_pin=17, dtoverlay=pwm, dtoverlay=spi1-1cs,cs0_pin=16`
- `overlays/w5500.dtbo, overlays/pwm.dtbo, overlays/spi1-1cs.dtbo` adicionados a `RPI_KERNEL_DEVICETREE_OVERLAYS`
- `PACKAGE_CLASSES = "package_deb"` e `package-management`; ajuste `PACKAGE_FEED_URIS` com o IP do servidor de pacotes
- `debug-tweaks` habilitado (root sem senha); remova para producao

### 5.2 Imagem `rpi-test-image`

O `rpi-test-image.bbappend` apenas faz `require` dos `.inc`, um por dominio:

| Arquivo | Conteudo |
|---|---|
| `packages` | pacotes extras, usuario `zero` (`extrausers`) e regra do grupo `sudo` |
| `network` | hostname, hosts, NetworkManager, DNS, Avahi, chrony, timezone e locale |
| `docker` | `docker.service` customizado, `docker-recover.service` e permissao do socket |
| `selfcheck` | `boot-selfcheck.service`, log de diagnostico do boot |
| `output` | nome dos artefatos (`ZERO_IMAGE_NAME`) e tipo `img` |

Servicos do layer habilitados no boot:

- `resize-rootfs.service`
- `docker.service` (customizado; `docker.socket` mascarado)
- `docker-recover.service`
- `docker-compose-app.service`
- `boot-selfcheck.service`

### 5.3 Rede (NetworkManager)

- `packagegroup-rpi-test.bbappend` remove o `connman`, que o `packagegroup-rpi-test` instala e que conflita com o NetworkManager
- `systemd_%.bbappend` remove `networkd`, `resolved`, `nss-resolve` e `timesyncd` do systemd, deixando NetworkManager e chrony como unicos responsaveis por rede, DNS e hora
- Pacotes: `networkmanager-nmcli`, `dnsmasq` e `avahi-daemon`
- DNS: lista em `DNS`, aplicada via `[global-dns-domain-*]` no `NetworkManager.conf`; o NetworkManager escreve o `/etc/resolv.conf` (`rc-manager=file`)
- Hostname `zero`, publicado por mDNS como `zero.local` pelo `avahi-daemon.service` do proprio pacote

### 5.4 Overlay PWM (exemplo)

O overlay vem do kernel do Raspberry Pi (para overlays proprios do layer, ver secao "Adicionar overlay de device tree"). Configuracao em `templates/local.conf`:

- `RPI_KERNEL_DEVICETREE_OVERLAYS:append = " overlays/pwm.dtbo "`: o `linux-raspberrypi` compila o `pwm.dtbo` e o `IMAGE_BOOT_FILES` o copia para `overlays/` na particao de boot (vfat)
- `dtoverlay=pwm` em `RPI_EXTRA_CONFIG`: o firmware carrega o overlay (GPIO 18, ALT5)

O firmware le `config.txt` e `.dtbo` apenas da particao de boot, montada em `/boot` no Linux. Arquivos instalados em `/boot` por pacotes do rootfs ficam encobertos por essa montagem e nao sao usados.

Para conferir no alvo:

```bash
findmnt /boot               # particao vfat (mmcblk0p1)
ls /boot/overlays/pwm.dtbo
ls /sys/class/pwm/          # pwmchip0 presente = overlay carregado
```

### 5.5 Resize automatico de rootfs

Recipe: `recipes-bsp/resize-rootfs/resize-rootfs.bb`

- Roda no primeiro boot (`resize-rootfs.service`, condicionado a ausencia de `/var/lib/resize-rootfs.done`)
- Expande a particao raiz para 100% do SD (`growpart` ou `parted`) e executa `resize2fs` (apenas ext4)
- Cria a flag `/var/lib/resize-rootfs.done` somente apos sucesso
- Log em `/var/log/resize-rootfs.log`

## 6) Flash da imagem

O build gera um `.img` bruto pronto para o **Raspberry Pi Imager** (opcao "Use custom"):

```text
tmp/deploy/images/raspberrypi0/zero-raspberrypi0.rootfs.img
```

O prefixo `zero` vem de `ZERO_IMAGE_NAME` (default em `rpi-test-image-output.inc`). Para trocar, defina em `templates/local.conf`:

```text
ZERO_IMAGE_NAME = "seu-nome-aqui"
```

Nao defina `IMAGE_NAME` diretamente: isso remove maquina, sufixo e data do nome e faz cada build sobrescrever o anterior.

Alternativa com `dd` (usando o `.wic.bz2`, mais compacto para transferir):

```bash
# troque /dev/sdX pelo dispositivo correto
sudo umount /dev/sdX* 2>/dev/null
bzcat tmp/deploy/images/raspberrypi0/zero-raspberrypi0.rootfs.wic.bz2 \
  | sudo dd of=/dev/sdX bs=4M conv=fsync status=progress
sudo eject /dev/sdX
```

## 7) Validacao no primeiro boot

Sequencia esperada:

1. Rootfs expandido automaticamente
2. `/etc/resolv.conf` com os servidores de `DNS`
3. Relogio sincronizado via chrony
4. `boot-selfcheck.service` registra o estado em `/var/log/boot-selfcheck.log`

Acesso: use `ssh zero@zero.local` (ou o IP que pegou da sua rede).

Checklist no alvo:

```bash
hostnamectl
nmcli general status
nmcli connection show
cat /etc/resolv.conf
chronyc tracking
systemctl status avahi-daemon docker-compose-app
docker ps
ls -l /dev/serial0 /dev/i2c-*
cat /var/log/boot-selfcheck.log
```

## 8) Como customizar

### DNS

Edite a variavel `DNS` em `rpi-test-image-network.inc`.

### Usuario/senha padrao

Edite `EXTRA_USERS_PARAMS` em `rpi-test-image-packages.inc` (usuario `zero`, senha `zero`, hash SHA-256).

### Nome dos artefatos

Defina `ZERO_IMAGE_NAME` (ver secao 6).

### Adicionar overlay de device tree

1. Se o overlay existe no kernel do Raspberry Pi, adicione `overlays/<nome>.dtbo` em `RPI_KERNEL_DEVICETREE_OVERLAYS`
2. Habilite com `dtoverlay=<nome>` em `RPI_EXTRA_CONFIG`

Overlay customizado (fora do kernel), como o `bq32000` (RTC I2C):

1. Coloque o `<nome>.dts` (com `/plugin/;`) em `recipes-bsp/overlays/files/` e adicione `file://<nome>.dts` ao `SRC_URI` de `recipes-bsp/overlays/overlays.bb`
2. Adicione `<nome>` a `ZERO_OVERLAYS` em `recipes-core/images/rpi-test-image-overlays.inc`
3. Habilite com `dtoverlay=<nome>` em `RPI_EXTRA_CONFIG`

A receita `overlays` (classe `devicetree`) compila o `.dtbo` e o publica em `${DEPLOY_DIR_IMAGE}/devicetree/`; o `.inc` da imagem o copia para `overlays/` na particao de boot via `IMAGE_BOOT_FILES`. Instalar o `.dtbo` em `/boot` pelo rootfs nao funciona (ver secao 5.4).

## 9) Troubleshooting

- `No match for argument` no bitbake: conferir se `meta-zero` esta no `BBLAYERS`
- Alteracao no `local.conf` sumiu: o `source-project.sh` sobrescreve com `templates/local.conf`
- Overlay nao aparece no boot: validar `dtoverlay=` no `config.txt` da particao de boot
- DNS diferente do esperado: conferir `[global-dns-domain-*]` em `/etc/NetworkManager/NetworkManager.conf` e rodar `nmcli general reload`
- Resize nao ocorreu: checar `/var/log/resize-rootfs.log` e a flag `/var/lib/resize-rootfs.done`
- Warning `tainted from a forced run`: aparece apos `bitbake -f ...`; para limpar, `bitbake -c cleansstate docker-compose-app rpi-test-image` e depois `bitbake rpi-test-image` sem `-f`

## 10) Observacoes de producao

- A imagem contem credenciais fixas (senha do usuario). Para producao, use provisionamento no primeiro boot ou segredos externos.
- Remova `debug-tweaks` do `local.conf` (habilita root sem senha).
- Versione e valide os `.dtbo` junto do kernel alvo para evitar incompatibilidade.
