option(host_build)

TARGET = QtBootstrap
QT =
CONFIG += minimal_syncqt internal_module force_bootstrap

MODULE_INCNAME = QtCore QtXml
MODULE_DEFINES = \
        QT_VERSION_STR=$$shell_quote(\"$$QT_VERSION\") \
        QT_VERSION_MAJOR=$$QT_MAJOR_VERSION \
        QT_VERSION_MINOR=$$QT_MINOR_VERSION \
        QT_VERSION_PATCH=$$QT_PATCH_VERSION \
        QT_BOOTSTRAPPED \
        QT_NO_CAST_TO_ASCII

DEFINES += \
    $$MODULE_DEFINES \
    QT_NO_FOREACH \
    QT_NO_CAST_FROM_ASCII

DEFINES -= QT_EVAL

SOURCES += \
           ../../corelib/codecs/qlatincodec.cpp \
           ../../corelib/codecs/qtextcodec.cpp \
           ../../corelib/codecs/qutfcodec.cpp \
           ../../corelib/global/qglobal.cpp \
           ../../corelib/global/qlogging.cpp \
           ../../corelib/global/qmalloc.cpp \
           ../../corelib/global/qnumeric.cpp \
           ../../corelib/io/qabstractfileengine.cpp \
           ../../corelib/io/qbuffer.cpp \
           ../../corelib/io/qdatastream.cpp \
           ../../corelib/io/qdebug.cpp \
           ../../corelib/io/qdir.cpp \
           ../../corelib/io/qdiriterator.cpp \
           ../../corelib/io/qfile.cpp \
           ../../corelib/io/qfileinfo.cpp \
           ../../corelib/io/qfilesystementry.cpp \
           ../../corelib/io/qfilesystemengine.cpp \
           ../../corelib/io/qfsfileengine.cpp \
           ../../corelib/io/qfsfileengine_iterator.cpp \
           ../../corelib/io/qiodevice.cpp \
           ../../corelib/io/qfiledevice.cpp \
           ../../corelib/io/qresource.cpp \
           ../../corelib/io/qtemporaryfile.cpp \
           ../../corelib/io/qtextstream.cpp \
           ../../corelib/io/qstandardpaths.cpp \
           ../../corelib/io/qloggingcategory.cpp \
           ../../corelib/io/qloggingregistry.cpp \
           ../../corelib/kernel/qcoreapplication.cpp \
           ../../corelib/kernel/qcoreglobaldata.cpp \
           ../../corelib/kernel/qmetatype.cpp \
           ../../corelib/kernel/qvariant.cpp \
           ../../corelib/kernel/qsystemerror.cpp \
           ../../corelib/plugin/quuid.cpp \
           ../../corelib/tools/qbitarray.cpp \
           ../../corelib/tools/qbytearray.cpp \
           ../../corelib/tools/qarraydata.cpp \
           ../../corelib/tools/qbytearraymatcher.cpp \
           ../../corelib/tools/qcommandlineparser.cpp \
           ../../corelib/tools/qcommandlineoption.cpp \
           ../../corelib/tools/qcryptographichash.cpp \
           ../../corelib/tools/qdatetime.cpp \
           ../../corelib/tools/qhash.cpp \
           ../../corelib/tools/qlist.cpp \
           ../../corelib/tools/qlinkedlist.cpp \
           ../../corelib/tools/qlocale.cpp \
           ../../corelib/tools/qlocale_tools.cpp \
           ../../corelib/tools/qmap.cpp \
           ../../corelib/tools/qregexp.cpp \
           ../../corelib/tools/qringbuffer.cpp \
           ../../corelib/tools/qpoint.cpp \
           ../../corelib/tools/qrect.cpp \
           ../../corelib/tools/qsize.cpp \
           ../../corelib/tools/qline.cpp \
           ../../corelib/tools/qstring.cpp \
           ../../corelib/tools/qstringbuilder.cpp \
           ../../corelib/tools/qstring_compat.cpp \
           ../../corelib/tools/qstringlist.cpp \
           ../../corelib/tools/qvector.cpp \
           ../../corelib/tools/qvsnprintf.cpp \
           ../../corelib/xml/qxmlutils.cpp \
           ../../corelib/xml/qxmlstream.cpp \
           ../../corelib/json/qjson.cpp \
           ../../corelib/json/qjsondocument.cpp \
           ../../corelib/json/qjsonobject.cpp \
           ../../corelib/json/qjsonarray.cpp \
           ../../corelib/json/qjsonvalue.cpp \
           ../../corelib/json/qjsonparser.cpp \
           ../../corelib/json/qjsonwriter.cpp \
           ../../xml/dom/qdom.cpp \
           ../../xml/sax/qxml.cpp

unix:SOURCES += ../../corelib/io/qfilesystemengine_unix.cpp \
                ../../corelib/io/qfilesystemiterator_unix.cpp \
                ../../corelib/io/qfsfileengine_unix.cpp

win32:SOURCES += ../../corelib/io/qfilesystemengine_win.cpp \
                 ../../corelib/io/qfilesystemiterator_win.cpp \
                 ../../corelib/io/qfsfileengine_win.cpp \
                 ../../corelib/kernel/qcoreapplication_win.cpp \
                 ../../corelib/plugin/qsystemlibrary.cpp \

mac {
    SOURCES += \
        ../../corelib/kernel/qcoreapplication_mac.cpp \
        ../../corelib/kernel/qcore_mac.cpp
    OBJECTIVE_SOURCES += \
        ../../corelib/kernel/qcore_mac_objc.mm \
        ../../corelib/kernel/qcore_foundation.mm

    LIBS += -framework Foundation
    osx: LIBS_PRIVATE += -framework CoreServices
    uikit: LIBS_PRIVATE += -framework UIKit
}

macx {
    OBJECTIVE_SOURCES += \
        ../../corelib/kernel/qcore_foundation.mm \
        ../../corelib/io/qstandardpaths_mac.mm
} else:unix {
    SOURCES += \
        ../../corelib/io/qstandardpaths_unix.cpp
} else {
    SOURCES += \
        ../../corelib/io/qstandardpaths_win.cpp
}

!qtConfig(system-zlib)|cross_compile {
    include(../../3rdparty/zlib.pri)
} else {
    CONFIG += no_core_dep
    include(../../3rdparty/zlib_dependency.pri)
}

win32 {
    LIBS += -luser32 -lole32 -ladvapi32 -lshell32
    mingw: LIBS += -luuid
}

load(qt_module)

# otherwise mingw headers do not declare common functions like putenv
mingw: CONFIG -= strict_c++

lib.CONFIG = dummy_install
INSTALLS += lib
