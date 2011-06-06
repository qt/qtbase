TEMPLATE = subdirs

FILES += \
    $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h \
    $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp

sources.files = $$FILES
sources.path = $$[QT_INSTALL_DATA]/platforms/fontdatabases/fontengines
INSTALLS = sources
