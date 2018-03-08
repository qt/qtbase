TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtConfig(egl_x11): SUBDIRS += eglfs_x11
qtConfig(eglfs_gbm): SUBDIRS *= eglfs_kms_support eglfs_kms
qtConfig(eglfs_egldevice): SUBDIRS *= eglfs_kms_support eglfs_kms_egldevice
qtConfig(eglfs_vsp2): SUBDIRS += eglfs_kms_vsp2
qtConfig(eglfs_brcm): SUBDIRS += eglfs_brcm
qtConfig(eglfs_mali): SUBDIRS += eglfs_mali
qtConfig(eglfs_viv): SUBDIRS += eglfs_viv
qtConfig(eglfs_rcar): SUBDIRS += eglfs_rcar
qtConfig(eglfs_viv_wl): SUBDIRS += eglfs_viv_wl
qtConfig(eglfs_openwfd): SUBDIRS += eglfs_openwfd
qtConfig(opengl): SUBDIRS += eglfs_emu

eglfs_kms_egldevice.depends = eglfs_kms_support
eglfs_kms.depends = eglfs_kms_support
