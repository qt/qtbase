# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QWasmIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QWasmIntegrationPlugin
    OUTPUT_NAME qwasm
    DEFAULT_IF "wasm" IN_LIST QT_QPA_PLATFORMS
    PLUGIN_TYPE platforms
    SOURCES
        main.cpp
        qwasmaccessibility.cpp qwasmaccessibility.h
        qwasmbase64iconstore.cpp qwasmbase64iconstore.h
        qwasmclipboard.cpp qwasmclipboard.h
        qwasmcompositor.cpp qwasmcompositor.h
        qwasmcssstyle.cpp qwasmcssstyle.h
        qwasmcursor.cpp qwasmcursor.h
        qwasmdom.cpp qwasmdom.h
        qwasmevent.cpp qwasmevent.h
        qwasmeventdispatcher.cpp qwasmeventdispatcher.h
        qwasmfontdatabase.cpp qwasmfontdatabase.h
        qwasmintegration.cpp qwasmintegration.h
        qwasmkeytranslator.cpp qwasmkeytranslator.h
        qwasmoffscreensurface.cpp qwasmoffscreensurface.h
        qwasmopenglcontext.cpp qwasmopenglcontext.h
        qwasmplatform.cpp qwasmplatform.h
        qwasmscreen.cpp qwasmscreen.h
        qwasmservices.cpp qwasmservices.h
        qwasmtheme.cpp qwasmtheme.h
        qwasmwindow.cpp qwasmwindow.h
        qwasmwindowclientarea.cpp qwasmwindowclientarea.h
        qwasmwindowtreenode.cpp qwasmwindowtreenode.h
        qwasmwindownonclientarea.cpp qwasmwindownonclientarea.h
        qwasminputcontext.cpp qwasminputcontext.h
        qwasmwindowstack.cpp qwasmwindowstack.h
        qwasmdrag.cpp qwasmdrag.h
    DEFINES
        QT_EGL_NO_X11
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    ATTRIBUTION_FILE_DIR_PATHS
        ../../../3rdparty/wasm
    QT_LICENSE_ID QT_COMMERCIAL_OR_GPL3
)

# Resources:
set(wasmfonts_resource_files
    "${QtBase_SOURCE_DIR}/src/3rdparty/wasm/DejaVuSans.ttf"
    "${QtBase_SOURCE_DIR}/src/3rdparty/wasm/DejaVuSansMono.ttf"
)

qt_internal_add_resource(QWasmIntegrationPlugin "wasmfonts"
    PREFIX
        "/fonts"
    BASE
        "${QtBase_SOURCE_DIR}/src/3rdparty/wasm"
    FILES
        ${wasmfonts_resource_files}
)

qt_internal_extend_target(QWasmIntegrationPlugin CONDITION QT_FEATURE_opengl
    SOURCES
        qwasmbackingstore.cpp qwasmbackingstore.h
    LIBRARIES
        Qt::OpenGL
        Qt::OpenGLPrivate
)

# PLUGIN_EXTENDS = "-"

set(wasm_support_files
    wasm_shell.html
    qtloader.js
    resources/qtlogo.svg
)

set(wasmwindow_resource_files
    "resources/maximize.svg"
    "resources/qtlogo.svg"
    "resources/restore.svg"
    "resources/x.svg"
)

qt_internal_add_resource(QWasmIntegrationPlugin "wasmwindow"
    PREFIX
        "/wasm-window"
    BASE
        "resources"
    FILES
        ${wasmwindow_resource_files}
)

qt_path_join(destination ${QT_INSTALL_DIR} "plugins/platforms")
qt_copy_or_install(FILES
    ${wasm_support_files}
    DESTINATION "${destination}"
)
# Need to copy the support files to the build dir in a top-level prefix build
# So _qt_internal_wasm_add_target_helpers finds them.
if(QT_WILL_INSTALL)
    foreach(wasm_support_file ${wasm_support_files})
        file(COPY "${wasm_support_file}" DESTINATION "${QT_BUILD_DIR}/plugins/platforms")
    endforeach()
endif()
