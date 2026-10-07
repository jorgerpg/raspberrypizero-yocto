SUMMARY = "Custom device tree overlays"
DESCRIPTION = "Custom Device Tree Overlays for Raspberry"
LICENSE = "CLOSED"

# Todo .dts com /plugin/ listado aqui e compilado para .dtbo e publicado em
# ${DEPLOY_DIR_IMAGE}/devicetree/
SRC_URI = " \
    file://bq32000.dts \
"

inherit devicetree

# devicetree.bbclass define COMPATIBLE_MACHINE ?= "^$" (nenhuma maquina)
COMPATIBLE_MACHINE = "^rpi$"
