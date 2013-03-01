android {
    QT += gui-private

    HEADERS += $$PWD/qjnihelpers_p.h \
               $$PWD/qjniobject_p.h

    SOURCES += $$PWD/qjnihelpers.cpp \
               $$PWD/qjniobject.cpp
}
