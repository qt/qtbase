#This file makes sure that harbuzz gets installed, 
#so that plugins can be compiled out of source
TEMPLATE = subdirs


SRCDIR += \
    $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

sources.files = $$SRCDIR
sources.path = $$[QT_INSTALL_DATA]/platforms/fontdatabases/harfbuzz
INSTALLS = sources

