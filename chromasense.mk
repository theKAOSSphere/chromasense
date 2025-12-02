######################################
# Buildroot package fragment for ChromaSense
######################################

CHROMASENSE_SITE_METHOD = local
CHROMASENSE_SITE = $($(PKG)_PKGDIR)/
CHROMASENSE_BUNDLES = chromasense.lv2

# version (bump to force rebuild if needed)
CHROMASENSE_VERSION = 1

# zita-resampler is now bundled in source, no external dependency needed
CHROMASENSE_DEPENDENCIES = fftw-single

# toolchain PATH trimmed to avoid inheriting Windows host entries with spaces
CHROMASENSE_TOOLCHAIN_PATH = $(HOST_DIR)/bin:$(HOST_DIR)/sbin:$(HOST_DIR)/usr/bin:$(HOST_DIR)/usr/sbin:/usr/bin:/bin

# Use standard variables for cross-compilation
CHROMASENSE_TARGET_MAKE = PATH="$(CHROMASENSE_TOOLCHAIN_PATH)" \
	CC="$(TARGET_CC)" CXX="$(TARGET_CXX)" AR="$(TARGET_AR)" LD="$(TARGET_LD)" \
	PKG_CONFIG="$(TARGET_PKG_CONFIG)" \
	CFLAGS="$(TARGET_CFLAGS)" CXXFLAGS="$(TARGET_CXXFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -C $(@D)/source

define CHROMASENSE_BUILD_CMDS
	$(CHROMASENSE_TARGET_MAKE)
endef

define CHROMASENSE_INSTALL_TARGET_CMDS
	$(CHROMASENSE_TARGET_MAKE) install DESTDIR=$(TARGET_DIR)
endef

$(eval $(generic-package))
