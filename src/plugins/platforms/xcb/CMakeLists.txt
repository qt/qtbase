# Generated from xcb.pro.

#####################################################################
## XcbQpaPrivate Module:
#####################################################################

qt_internal_add_module(XcbQpaPrivate
    CONFIG_MODULE_NAME xcb_qpa_lib
    INTERNAL_MODULE
    NO_MODULE_HEADERS
    SOURCES
        gl_integrations/qxcbglintegration.cpp gl_integrations/qxcbglintegration.h
        gl_integrations/qxcbglintegrationfactory.cpp gl_integrations/qxcbglintegrationfactory.h
        gl_integrations/qxcbglintegrationplugin.h
        gl_integrations/qxcbnativeinterfacehandler.cpp gl_integrations/qxcbnativeinterfacehandler.h
        qxcbatom.cpp qxcbatom.h
        qxcbbackingstore.cpp qxcbbackingstore.h
        qxcbclipboard.cpp qxcbclipboard.h
        qxcbconnection.cpp qxcbconnection.h
        qxcbconnection_basic.cpp qxcbconnection_basic.h
        qxcbconnection_screens.cpp
        qxcbconnection_xi2.cpp
        qxcbcursor.cpp qxcbcursor.h
        qxcbeventdispatcher.cpp qxcbeventdispatcher.h
        qxcbeventqueue.cpp qxcbeventqueue.h
        qxcbimage.cpp qxcbimage.h
        qxcbintegration.cpp qxcbintegration.h
        qxcbkeyboard.cpp qxcbkeyboard.h
        qxcbscrollingdevice.cpp qxcbscrollingdevice_p.h
        qxcbmime.cpp qxcbmime.h
        qxcbnativeinterface.cpp qxcbnativeinterface.h
        qxcbobject.h
        qxcbscreen.cpp qxcbscreen.h
        qxcbsystemtraytracker.cpp qxcbsystemtraytracker.h
        qxcbwindow.cpp qxcbwindow.h
        qxcbwmsupport.cpp qxcbwmsupport.h
        qxcbxsettings.cpp qxcbxsettings.h
    DEFINES
        QT_BUILD_XCB_PLUGIN
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        gl_integrations
    PUBLIC_LIBRARIES
        PkgConfig::XKB_COMMON_X11
        Qt::CorePrivate
        Qt::GuiPrivate
        XCB::ICCCM
        XCB::IMAGE
        XCB::KEYSYMS
        XCB::RANDR
        XCB::RENDER
        XCB::RENDERUTIL
        XCB::SHAPE
        XCB::SHM
        XCB::SYNC
        XCB::XCB
        XCB::XFIXES
        # XCB::XINPUT # special case remove handled below
        XCB::XKB
        XKB::XKB
)

# special case begin
qt_disable_apple_app_extension_api_only(XcbQpaPrivate)
# special case end

## Scopes:
#####################################################################

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_opengl
    PUBLIC_LIBRARIES
        Qt::OpenGLPrivate
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_glib
    LIBRARIES
        GLIB2::GLIB2
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_draganddrop
    SOURCES
        qxcbdrag.cpp qxcbdrag.h
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_xcb_xlib
    PUBLIC_LIBRARIES
        X11::XCB
        # special case begin
        # 'QMAKE_USE += xcb_xlib' in qmake implies also += xlib (aka X11)
        # due to "use": "xcb xlib" in src/gui/configure.json.
        # That's not yet handled by the conversion scripts unfortunately.
        X11::X11
        # special case end
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_xcb_sm
    SOURCES
        qxcbsessionmanager.cpp qxcbsessionmanager.h
    PUBLIC_LIBRARIES
        X11::SM
        X11::ICE
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_vulkan
    SOURCES
        qxcbvulkaninstance.cpp qxcbvulkaninstance.h
        qxcbvulkanwindow.cpp qxcbvulkanwindow.h
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_dlopen
    PUBLIC_LIBRARIES
        ${CMAKE_DL_LIBS}
)

qt_internal_extend_target(XcbQpaPrivate CONDITION CLANG AND NOT ICC
    COMPILE_OPTIONS
        -ftemplate-depth=1024
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_xcb_native_painting
    SOURCES
        nativepainting/qbackingstore_x11.cpp nativepainting/qbackingstore_x11_p.h
        nativepainting/qcolormap_x11.cpp nativepainting/qcolormap_x11_p.h
        nativepainting/qpaintengine_x11.cpp nativepainting/qpaintengine_x11_p.h
        nativepainting/qpixmap_x11.cpp nativepainting/qpixmap_x11_p.h
        nativepainting/qpolygonclipper_p.h
        nativepainting/qt_x11_p.h
        nativepainting/qtessellator.cpp nativepainting/qtessellator_p.h
        nativepainting/qxcbnativepainting.cpp nativepainting/qxcbnativepainting.h
    INCLUDE_DIRECTORIES
        nativepainting
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_xcb_native_painting AND QT_FEATURE_xrender
    PUBLIC_LIBRARIES
        PkgConfig::XRender
)

qt_internal_extend_target(XcbQpaPrivate CONDITION QT_FEATURE_fontconfig AND QT_FEATURE_xcb_native_painting
    LIBRARIES
        WrapFreetype::WrapFreetype
)

# special case begin
if(QT_FEATURE_system_xcb_xinput)
    qt_internal_extend_target(XcbQpaPrivate LIBRARIES XCB::XINPUT)
else()
    set(xinput_source "${PROJECT_SOURCE_DIR}/src/3rdparty/xcb/libxcb/xinput.c")
    set_source_files_properties(
        "${xinput_source}"
        PROPERTIES COMPILE_OPTIONS "-w"
    )
    target_sources(XcbQpaPrivate PRIVATE "${xinput_source}")
    target_include_directories(XcbQpaPrivate PRIVATE
        "${PROJECT_SOURCE_DIR}/src/3rdparty/xcb/include"
    )
endif()
# special case end

#####################################################################
## QXcbIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QXcbIntegrationPlugin
    OUTPUT_NAME qxcb
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES xcb # special case
    SOURCES
        qxcbmain.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::XcbQpaPrivate
)

#### Keys ignored in scope 18:.:.:xcb-plugin.pro:<TRUE>:
# OTHER_FILES = "xcb.json" "README"

## Scopes:
#####################################################################

#### Keys ignored in scope 20:.:.:xcb-plugin.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"
add_subdirectory(gl_integrations)
if(OFF)
    add_subdirectory(xcb-static) # special case TODO: xcb-static sub folder
endif()

