# Note: OpenGL32 must precede Gdi32 as it overwrites some functions.
LIBS *= -lole32
!wince: LIBS *= -luser32 -lwinspool -limm32 -lwinmm -loleaut32

contains(QT_CONFIG, opengl):!contains(QT_CONFIG, opengles2):!contains(QT_CONFIG, dynamicgl): LIBS *= -lopengl32

mingw: LIBS *= -luuid
# For the dialog helpers:
!wince: LIBS *= -lshlwapi -lshell32
!wince: LIBS *= -ladvapi32
wince: DEFINES *= QT_LIBINFIX=L"\"\\\"$${QT_LIBINFIX}\\\"\""

DEFINES *= QT_NO_CAST_FROM_ASCII

contains(QT_CONFIG, directwrite) {
    SOURCES += $$PWD/qwindowsfontenginedirectwrite.cpp
    HEADERS += $$PWD/qwindowsfontenginedirectwrite.h
} else {
    DEFINES *= QT_NO_DIRECTWRITE
}

SOURCES += \
    $$PWD/qwindowswindow.cpp \
    $$PWD/qwindowsintegration.cpp \
    $$PWD/qwindowscontext.cpp \
    $$PWD/qwindowsscreen.cpp \
    $$PWD/qwindowskeymapper.cpp \
    $$PWD/qwindowsfontengine.cpp \
    $$PWD/qwindowsfontdatabase.cpp \
    $$PWD/qwindowsmousehandler.cpp \
    $$PWD/qwindowsole.cpp \
    $$PWD/qwindowsmime.cpp \
    $$PWD/qwindowsinternalmimedata.cpp \
    $$PWD/qwindowscursor.cpp \
    $$PWD/qwindowsinputcontext.cpp \
    $$PWD/qwindowstheme.cpp \
    $$PWD/qwindowsdialoghelpers.cpp \
    $$PWD/qwindowsservices.cpp \
    $$PWD/qwindowsnativeimage.cpp \
    $$PWD/qwindowsnativeinterface.cpp \
    $$PWD/qwindowsopengltester.cpp

HEADERS += \
    $$PWD/qwindowswindow.h \
    $$PWD/qwindowsintegration.h \
    $$PWD/qwindowscontext.h \
    $$PWD/qwindowsscreen.h \
    $$PWD/qwindowskeymapper.h \
    $$PWD/qwindowsfontengine.h \
    $$PWD/qwindowsfontdatabase.h \
    $$PWD/qwindowsmousehandler.h \
    $$PWD/qtwindowsglobal.h \
    $$PWD/qtwindows_additional.h \
    $$PWD/qwindowsole.h \
    $$PWD/qwindowsmime.h \
    $$PWD/qwindowsinternalmimedata.h \
    $$PWD/qwindowscursor.h \
    $$PWD/array.h \
    $$PWD/qwindowsinputcontext.h \
    $$PWD/qwindowstheme.h \
    $$PWD/qwindowsdialoghelpers.h \
    $$PWD/qwindowsservices.h \
    $$PWD/qplatformfunctions_wince.h \
    $$PWD/qwindowsnativeimage.h \
    $$PWD/qwindowsnativeinterface.h \
    $$PWD/qwindowsopengltester.h

INCLUDEPATH += $$PWD

contains(QT_CONFIG,opengl): HEADERS += $$PWD/qwindowsopenglcontext.h

contains(QT_CONFIG, opengles2) {
    SOURCES += $$PWD/qwindowseglcontext.cpp
    HEADERS += $$PWD/qwindowseglcontext.h
} else: contains(QT_CONFIG,opengl) {
    SOURCES += $$PWD/qwindowsglcontext.cpp
    HEADERS += $$PWD/qwindowsglcontext.h
}

# Dynamic GL needs both WGL and EGL
contains(QT_CONFIG,dynamicgl) {
    SOURCES += $$PWD/qwindowseglcontext.cpp
    HEADERS += $$PWD/qwindowseglcontext.h
}

!contains( DEFINES, QT_NO_CLIPBOARD ) {
    SOURCES += $$PWD/qwindowsclipboard.cpp
    HEADERS += $$PWD/qwindowsclipboard.h
}

# drag and drop on windows only works if a clipboard is available
!contains( DEFINES, QT_NO_DRAGANDDROP ) {
    !win32:SOURCES += $$PWD/qwindowsdrag.cpp
    !win32:HEADERS += $$PWD/qwindowsdrag.h
    win32:!contains( DEFINES, QT_NO_CLIPBOARD ) {
        HEADERS += $$PWD/qwindowsdrag.h
        SOURCES += $$PWD/qwindowsdrag.cpp
    }
}

!wince:!contains( DEFINES, QT_NO_TABLETEVENT ) {
    INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/wintab
    HEADERS += $$PWD/qwindowstabletsupport.h
    SOURCES += $$PWD/qwindowstabletsupport.cpp
}

!wince:!contains( DEFINES, QT_NO_SESSIONMANAGER ) {
    SOURCES += $$PWD/qwindowssessionmanager.cpp
    HEADERS += $$PWD/qwindowssessionmanager.h
}

!wince:!contains( DEFINES, QT_NO_IMAGEFORMAT_PNG ) {
    RESOURCES += $$PWD/cursors.qrc
}

!wince: RESOURCES += $$PWD/openglblacklists.qrc

contains(QT_CONFIG, freetype) {
    DEFINES *= QT_NO_FONTCONFIG
    include($$QT_SOURCE_TREE/src/3rdparty/freetype_dependency.pri)
    HEADERS += \
               $$PWD/qwindowsfontdatabase_ft.h
    SOURCES += \
               $$PWD/qwindowsfontdatabase_ft.cpp
} else:contains(QT_CONFIG, system-freetype) {
    include($$QT_SOURCE_TREE/src/platformsupport/fontdatabases/basic/basic.pri)
    HEADERS += \
               $$PWD/qwindowsfontdatabase_ft.h
    SOURCES += \
               $$PWD/qwindowsfontdatabase_ft.cpp
}

contains(QT_CONFIG, accessibility):include($$PWD/accessible/accessible.pri)

DEFINES *= LIBEGL_NAME=$${LIBEGL_NAME}
DEFINES *= LIBGLESV2_NAME=$${LIBGLESV2_NAME}
