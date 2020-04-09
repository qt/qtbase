TEMPLATE = subdirs
SUBDIRS = \
        qfile_vs_qnetworkaccessmanager \
        qnetworkreply \
        qnetworkreply_from_cache \
        qnetworkdiskcache

qtConfig(private_tests): \
    SUBDIRS += \
        qdecompresshelper \

