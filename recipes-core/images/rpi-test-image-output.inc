# Output naming and .img artifact
#
# Set ZERO_IMAGE_NAME in local.conf to rename every generated image
# artifact (manifest, wic.bz2, img, etc), e.g.:
#   ZERO_IMAGE_NAME = "meu-produto"
# IMAGE_NAME itself can't be used for this: it is derived from IMAGE_BASENAME
# (IMAGE_NAME -> IMAGE_LINK_NAME -> IMAGE_BASENAME), so pointing IMAGE_BASENAME
# back at it would expand recursively.
ZERO_IMAGE_NAME ?= "zero"
IMAGE_BASENAME = "${ZERO_IMAGE_NAME}"

# Raspberry Pi Imager loads raw .img files directly with no decompression
# support needed. The wic type dependency builds the disk image once and the
# raw copy is taken from it.
IMAGE_FSTYPES += "img"
IMAGE_TYPEDEP:img = "wic"
IMAGE_CMD:img = "cp --reflink=auto ${IMGDEPLOYDIR}/${IMAGE_NAME}.wic ${IMGDEPLOYDIR}/${IMAGE_NAME}.img"
