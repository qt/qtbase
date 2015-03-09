TEMPLATE = subdirs

contains(QT_CONFIG, egl_x11): SUBDIRS += eglfs_x11
contains(QT_CONFIG, kms): SUBDIRS += eglfs_kms
contains(QT_CONFIG, eglfs_brcm): SUBDIRS += eglfs_brcm
contains(QT_CONFIG, eglfs_mali): SUBDIRS += eglfs_mali
contains(QT_CONFIG, eglfs_viv): SUBDIRS += eglfs_viv
