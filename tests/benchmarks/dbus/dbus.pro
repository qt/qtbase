TEMPLATE = subdirs
SUBDIRS = \
        qdbustype

qtConfig(process): SUBDIRS += \
        qdbusperformance
