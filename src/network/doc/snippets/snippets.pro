TEMPLATE = subdirs
TARGET = network_cppsnippets
SUBDIRS =

contains(QT_BUILD_PARTS, tests) {
    SUBDIRS += \
        network
}

