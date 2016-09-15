TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtConfig(egl_x11): SUBDIRS += eglfs_x11
qtConfig(eglfs_gbm): SUBDIRS += eglfs_kms_support eglfs_kms
qtConfig(eglfs_egldevice): SUBDIRS += eglfs_kms_support eglfs_kms_egldevice
qtConfig(eglfs_brcm): SUBDIRS += eglfs_brcm
qtConfig(eglfs_mali): SUBDIRS += eglfs_mali
qtConfig(eglfs_viv): SUBDIRS += eglfs_viv
qtConfig(eglfs_viv_wl): SUBDIRS += eglfs_viv_wl

eglfs_kms_egldevice.depends = eglfs_kms_support
eglfs_kms.depends = eglfs_kms_support
