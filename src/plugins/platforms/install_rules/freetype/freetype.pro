TEMPLATE = subdirs

SRCDIR += \
    $$QT_SOURCE_TREE/src/3rdparty/freetype/src

INCDIR += \
    $$QT_SOURCE_TREE/src/3rdparty/freetype/include

sources.files = $$SRCDIR $$INCDIR
sources.path = $$[QT_INSTALL_DATA]/platforms/fontdatabases/freetype
INSTALLS = sources
