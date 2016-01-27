TEMPLATE = subdirs

contains(QT_CONFIG, egl_x11): SUBDIRS += eglfs_x11
contains(QT_CONFIG, eglfs_gbm): SUBDIRS += eglfs_kms
contains(QT_CONFIG, eglfs_egldevice): SUBDIRS += eglfs_kms_egldevice
contains(QT_CONFIG, eglfs_brcm): SUBDIRS += eglfs_brcm
contains(QT_CONFIG, eglfs_mali): SUBDIRS += eglfs_mali
contains(QT_CONFIG, eglfs_viv): SUBDIRS += eglfs_viv
contains(QT_CONFIG, eglfs_viv_wl): SUBDIRS += eglfs_viv_wl
