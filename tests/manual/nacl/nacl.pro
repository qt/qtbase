TEMPLATE = subdirs
# subdirs roughly in order of complexity
SUBDIRS = \ 
    qmake \                         # build a standard ppapi "hello world" example with qmake.
    qtcore \                        # simple QtCore usage (qstring).
    prototype_main \                # qt_main app startup prototype.
    prototype_openglfucntions \     # resolving opengl functions prototyping.
    prototype_libppapimain \        # call PpapiPluginMain from a shared library.
    window_raster \                 # QWindow with raster graphics.
    window_opengl \                 # QWindow with OpenGL graphics.
    window_qtquick \                # QQuickWindow
