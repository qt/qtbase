# These defines might be necessary if your DirectFB driver doesn't
# support all of the DirectFB API.
#
#DEFINES += QT_DIRECTFB_SUBSURFACE
#DEFINES += QT_DIRECTFB_WINDOW_AS_CURSOR
#DEFINES += QT_NO_DIRECTFB_IMAGEPROVIDER
#DEFINES += QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
#DEFINES += QT_DIRECTFB_IMAGECACHE
#DEFINES += QT_NO_DIRECTFB_WM
#DEFINES += QT_NO_DIRECTFB_LAYER
#DEFINES += QT_DIRECTFB_PALETTE
#DEFINES += QT_NO_DIRECTFB_PREALLOCATED
#DEFINES += QT_NO_DIRECTFB_MOUSE
#DEFINES += QT_NO_DIRECTFB_KEYBOARD
#DEFINES += QT_DIRECTFB_TIMING
#DEFINES += QT_NO_DIRECTFB_OPAQUE_DETECTION
#DEFINES += QT_NO_DIRECTFB_STRETCHBLIT
DIRECTFB_DRAWINGOPERATIONS=DRAW_RECTS|DRAW_LINES|DRAW_IMAGE|DRAW_PIXMAP|DRAW_TILED_PIXMAP|STROKE_PATH|DRAW_PATH|DRAW_POINTS|DRAW_ELLIPSE|DRAW_POLYGON|DRAW_TEXT|FILL_PATH|FILL_RECT|DRAW_COLORSPANS|DRAW_ROUNDED_RECT|DRAW_STATICTEXT
#DEFINES += \"QT_DIRECTFB_WARN_ON_RASTERFALLBACKS=$$DIRECTFB_DRAWINGOPERATIONS\"
#DEFINES += \"QT_DIRECTFB_DISABLE_RASTERFALLBACKS=$$DIRECTFB_DRAWINGOPERATIONS\"

HEADERS += $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbscreen.h \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbwindowsurface.h \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbpaintengine.h \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbpaintdevice.h \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbpixmap.h \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbkeyboard.h \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbmouse.h

SOURCES += $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbscreen.cpp \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbwindowsurface.cpp \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbpaintengine.cpp \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbpaintdevice.cpp \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbpixmap.cpp \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbkeyboard.cpp \
           $$QT_SOURCE_TREE/src/plugins/gfxdrivers/directfb/qdirectfbmouse.cpp


QMAKE_CXXFLAGS += $$QT_CFLAGS_DIRECTFB
LIBS += $$QT_LIBS_DIRECTFB
