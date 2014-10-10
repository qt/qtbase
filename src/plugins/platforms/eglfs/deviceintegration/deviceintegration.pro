TEMPLATE = subdirs

contains(QT_CONFIG, egl_x11): SUBDIRS += eglfs_x11
contains(QT_CONFIG, kms): SUBDIRS += eglfs_kms
