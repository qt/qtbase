if(ANDROID) # AND NOT android-embedded
#    TODO   add_subdirectory(android)
endif()

if(NOT ANDROID)
    add_subdirectory(minimal)
endif()

if (NOT ANDROID AND QT_FEATURE_freetype)
    add_subdirectory(offscreen)
endif()

if(QT_FEATURE_xcb)
    add_subdirectory(xcb)
endif()

if(APPLE_UIKIT AND NOT watchOS)
#    TODO   add_subdirectory(ios)
endif()

if(APPLE_OSX)
    add_subdirectory(cocoa)
endif()

if(WIN32 AND NOT WINRT)
#    TODO   add_subdirectory(windows)
endif()

if(WINRT)
#    TODO   add_subdirectory(winrt)
endif()

if(QT_FEATURE_direct2d)
#    TODO   add_subdirectory(direct2d)
endif()

if(QNX)
#    TODO   add_subdirectory(qnx)
endif()

if(QT_FEATURE_eglfs)
    add_subdirectory(eglfs)
    add_subdirectory(minimalegl)
endif()

if(QT_FEATURE_directfb)
#    TODO   add_subdirectory(directfb)
endif()

if(QT_FEATURE_linuxfb)
    add_subdirectory(linuxfb)
endif()

if(TARGET Qt::Network AND QT_FEATURE_vnc)
    add_subdirectory(vnc)
endif()

if(FREEBSD)
#    TODO   add_subdirectory(bsdfb)
endif()

if(HAIKU)
#    TODO   add_subdirectory(haiku)
endif()

#if(WASM)
#    TODO   add_subdirectory(wasm)
#endif()

if(QT_FEATURE_mirclient)
#    TODO   add_subdirectory(mirclient)
endif()

if(QT_FEATURE_integrityfb)
#    TODO   add_subdirectory(integrityfb)
endif()
