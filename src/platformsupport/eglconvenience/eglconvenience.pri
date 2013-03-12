contains(QT_CONFIG,egl) {
    HEADERS += \
        $$PWD/qeglconvenience_p.h \
        $$PWD/qeglplatformcontext_p.h
    SOURCES += \
        $$PWD/qeglconvenience.cpp \
        $$PWD/qeglplatformcontext.cpp

    contains(QT_CONFIG,xlib) {
        HEADERS += \
            $$PWD/qxlibeglintegration_p.h
        SOURCES += \
            $$PWD/qxlibeglintegration.cpp
    }
    CONFIG += egl
}

