# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(WrapJpeg PROVIDED_TARGETS WrapJpeg::WrapJpeg)

#####################################################################
## QJpegPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QJpegPlugin
    OUTPUT_NAME qjpeg
    PLUGIN_TYPE imageformats
    SOURCES
        main.cpp
        qjpeghandler.cpp qjpeghandler_p.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        WrapJpeg::WrapJpeg
)

# Fails to build on Windows with a static Qt, PCH enabled and
# the vendored libjpeg sources, due to 'boolean'
# redefinition in jmorecfg.h and rpcndr.h.
if(WIN32 AND NOT BUILD_SHARED_LIBS)
    qt_update_ignore_pch_source(QJpegPlugin "qjpeghandler.cpp")
endif()
