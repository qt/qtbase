# Generated from wasm.pro.

#####################################################################
## QWasmIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QWasmIntegrationPlugin
    OUTPUT_NAME qwasm
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES wasm # special case
    PLUGIN_TYPE platforms
    STATIC
    SOURCES
        main.cpp
        qwasmclipboard.cpp qwasmclipboard.h
        qwasmcompositor.cpp qwasmcompositor.h
        qwasmcursor.cpp qwasmcursor.h
        qwasmeventdispatcher.cpp qwasmeventdispatcher.h
        qwasmeventtranslator.cpp qwasmeventtranslator.h
        qwasmfontdatabase.cpp qwasmfontdatabase.h
        qwasmintegration.cpp qwasmintegration.h
        qwasmoffscreensurface.cpp qwasmoffscreensurface.h
        qwasmopenglcontext.cpp qwasmopenglcontext.h
        qwasmscreen.cpp qwasmscreen.h
        qwasmservices.cpp qwasmservices.h
        qwasmstring.cpp qwasmstring.h
        qwasmstylepixmaps_p.h
        qwasmtheme.cpp qwasmtheme.h
        qwasmwindow.cpp qwasmwindow.h
    DEFINES
        QT_EGL_NO_X11
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(wasmfonts_resource_files
    "${QtBase_SOURCE_DIR}/src/3rdparty/wasm/Vera.ttf"
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

#### Keys ignored in scope 4:.:.:wasm.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

qt_copy_or_install(FILES
    wasm_shell.html
    DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins/platforms/"
)
qt_copy_or_install(FILES
    qtloader.js
    DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins/platforms/"
)
qt_copy_or_install(FILES
    qtlogo.svg
    DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins/platforms/"
)
