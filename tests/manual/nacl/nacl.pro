TEMPLATE = subdirs
# subdirs roughly in order of complexity
SUBDIRS = \ 
    qmake \                         # build a standard ppapi "hello world" example with qmake.
    qtcore \                        # simple QtCore usage (qstring).
    resources \                     # test the qrc resources system
    urlload \                       # test url loading
    prototype_main \                # qt_main app startup prototype.
    prototype_openglfucntions \     # resolving opengl functions prototyping.
    prototype_libppapimain \        # call PpapiPluginMain from a shared library.
    worker_thread \                 # Test the Qt event dispatcher on a worker thread.
    window_raster \                 # QWindow with raster graphics.
    window_opengl \                 # QWindow with OpenGL graphics.
    window_qtquick \                # QQuickWindow
    window_qmlapplicationengine \   # QQmlApplicationEngine
    window_shadereffects \          # Shaders!
    window_controls \               # (some) Qt Quick Controls
    window_controls_gallery \       # Qt Quick Gallery example
