TEMPLATE = subdirs
SUBDIRS += src

# We need to call syncqt manually instead of using "load(qt_module_headers)" for several reasons:
#  1) qt_module_headers assumes the TARGET is the same as the include directory (eg: libGLESv2 != GLES2)
#  2) If we made a 'QtANGLE' module, the include directory would be flattened which won't work since
#     we need to support "#include <GLES2/gl2.h>"
!build_pass {
    qtPrepareTool(QMAKE_SYNCQT, syncqt)
    QTDIR = $$[QT_HOST_PREFIX]
    exists($$QTDIR/.qmake.cache): \
        mod_component_base = $$QTDIR
    else: \
        mod_component_base = $$dirname(_QMAKE_CACHE_)
    QMAKE_SYNCQT += -minimal -module KHR -module EGL -module GLES2 \
        -mkspecsdir $$[QT_HOST_DATA/get]/mkspecs -outdir $$mod_component_base $$dirname(_QMAKE_CONF_)
    !silent:message($$QMAKE_SYNCQT)
    system($$QMAKE_SYNCQT)|error("Failed to run: $$QMAKE_SYNCQT")
}
