SOURCES = journald.c

CONFIG += link_pkgconfig

packagesExist(libsystemd): \
    PKGCONFIG_PRIVATE += libsystemd
else: \
    PKGCONFIG_PRIVATE += libsystemd-journal

CONFIG -= qt
