# Generated from manual.pro.

# special case begin
# Don't build manual tests when targeting iOS.
if(UIKIT)
    return()
endif()
# special case end

add_subdirectory(filetest)
# special case begin
# diaglib is broken in dev due to missing
# QtOpenGL/QGLFunctions headers
# add_subdirectory(embeddedintoforeignwindow)
# add_subdirectory(foreignwindows)
# special case end
add_subdirectory(gestures)
# add_subdirectory(highdpi) special case broken in dev
add_subdirectory(inputmethodhints)
add_subdirectory(keypadnavigation)
#add_subdirectory(lance) # special case qgl.h missing
add_subdirectory(qcursor)
add_subdirectory(qdesktopservices)
add_subdirectory(qgraphicsitem)
add_subdirectory(qgraphicsitemgroup)
add_subdirectory(qgraphicslayout/flicker)
add_subdirectory(qhttpnetworkconnection)
add_subdirectory(qimagereader)
add_subdirectory(qlayout)
add_subdirectory(qlocale)
add_subdirectory(qmimedatabase)
add_subdirectory(qnetconmonitor)
add_subdirectory(qnetworkaccessmanager/qget)
add_subdirectory(qnetworkinformation)
#special case begin
if (QT_FEATURE_openssl AND UNIX)
    add_subdirectory(qnetworkreply)
endif()
#special case end
add_subdirectory(qstorageinfo)
add_subdirectory(qscreen)
add_subdirectory(qscreen_xrandr)
add_subdirectory(qsslsocket)
add_subdirectory(qsysinfo)
add_subdirectory(qtabletevent)
add_subdirectory(qtexteditlist)
add_subdirectory(qtexttableborders)
add_subdirectory(qtbug-8933)
add_subdirectory(qtbug-52641)
add_subdirectory(qtouchevent)
add_subdirectory(touch)
add_subdirectory(qwidget_zorder)
add_subdirectory(repaint)
add_subdirectory(socketengine)
add_subdirectory(textrendering)
add_subdirectory(widgets)
add_subdirectory(windowflags)
add_subdirectory(windowgeometry)
add_subdirectory(windowmodality)
add_subdirectory(widgetgrab)
add_subdirectory(xembed-raster)
add_subdirectory(xembed-widgets)
add_subdirectory(shortcuts)
add_subdirectory(dialogs)
add_subdirectory(windowtransparency)
add_subdirectory(unc)
add_subdirectory(qtabbar)
add_subdirectory(rhi)
if(UNIX)
    add_subdirectory(network_remote_stresstest)
    add_subdirectory(network_stresstest)
endif()
if(QT_FEATURE_openssl)
    add_subdirectory(qssloptions)
endif()
if(QT_FEATURE_opengl)
    # add_subdirectory(qopengltextureblitter) special case broken in dev
endif()
if(QT_FEATURE_egl AND QT_FEATURE_opengl)
    # add_subdirectory(qopenglcontext) # special case broken in dev
endif()
if(QT_FEATURE_vulkan)
    add_subdirectory(qvulkaninstance)
endif()
