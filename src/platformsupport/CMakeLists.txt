# Generated from platformsupport.pro.

add_subdirectory(themes)
add_subdirectory(edid)
add_subdirectory(eventdispatchers)
add_subdirectory(devicediscovery)
add_subdirectory(fbconvenience)

if(QT_FEATURE_freetype OR APPLE OR WIN32)
    add_subdirectory(fontdatabases)
endif()

if((UNIX AND (NOT uikit)) OR QT_FEATURE_xcb)
    add_subdirectory(services)
endif()

if(QT_FEATURE_evdev OR QT_FEATURE_tslib OR QT_FEATURE_libinput OR QT_FEATURE_integrityhid)
    add_subdirectory(input)
endif()

if(QT_FEATURE_accessibility)
    add_subdirectory(accessibility)
    if(QT_FEATURE_accessibility_atspi_bridge)
        add_subdirectory(linuxaccessibility)
    endif()
    if(WIN32 AND NOT WINRT)
        add_subdirectory(windowsuiautomation)
    endif()
endif()

if(QT_FEATURE_vulkan)
    add_subdirectory(vkconvenience)
endif()

if(QT_FEATURE_kms)
    add_subdirectory(kmsconvenience)
endif()

if(QT_FEATURE_egl)
    add_subdirectory(eglconvenience)
endif()

if(QT_FEATURE_opengl)
    add_subdirectory(platformcompositor)
endif()

if (QT_FEATURE_xlib AND QT_FEATURE_opengl AND TARGET OpenGL::GLX) # special case
    add_subdirectory(glxconvenience)
endif()

if(APPLE)
    add_subdirectory(clipboard)
    add_subdirectory(graphics)
endif()
