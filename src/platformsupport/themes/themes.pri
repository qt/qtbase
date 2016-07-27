unix:!mac {
    include($$PWD/genericunix/genericunix.pri)
}

HEADERS += \
    $$PWD/qabstractfileiconengine_p.h

SOURCES += \
    $$PWD/qabstractfileiconengine.cpp
