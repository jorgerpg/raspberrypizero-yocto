# Overlays customizados (recipes-bsp/overlays)
#
# Para cada .dts novo em recipes-bsp/overlays/files: adicione-o ao SRC_URI da
# receita, a ZERO_OVERLAYS abaixo e habilite com dtoverlay=<nome> em
# RPI_EXTRA_CONFIG (templates/local.conf).
ZERO_OVERLAYS ?= "bq32000"

# A receita publica em ${DEPLOY_DIR_IMAGE}/devicetree/; o firmware le de
# overlays/ na particao de boot.
IMAGE_BOOT_FILES:append = " ${@' '.join('devicetree/%s.dtbo;overlays/%s.dtbo' % (o, o) for o in d.getVar('ZERO_OVERLAYS').split())}"

# Garante que os .dtbo existam no deploy antes da montagem da particao de boot
do_image_wic[depends] += "overlays:do_deploy"
