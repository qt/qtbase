option(host_build)

TARGET = QtBootstrap
QT =
CONFIG += minimal_syncqt internal_module force_bootstrap gc_binaries

MODULE_INCNAME = QtCore QtXml
MODULE_DEFINES = \
        QT_VERSION_STR=$$shell_quote(\"$$QT_VERSION\") \
        QT_VERSION_MAJOR=$$QT_MAJOR_VERSION \
        QT_VERSION_MINOR=$$QT_MINOR_VERSION \
        QT_VERSION_PATCH=$$QT_PATCH_VERSION \
        QT_BOOTSTRAPPED \
        QT_NO_CAST_TO_ASCII
MODULE_CONFIG = gc_binaries

DEFINES += \
    $$MODULE_DEFINES \
    QT_NO_FOREACH \
    QT_NO_CAST_FROM_ASCII

INCLUDEPATH += \
    $$PWD/.. \
    $$PWD/../../3rdparty/tinycbor/src

SOURCES += \
           ../../corelib/codecs/qlatincodec.cpp \
           ../../corelib/codecs/qtextcodec.cpp \
           ../../corelib/codecs/qutfcodec.cpp \
           ../../corelib/global/qendian.cpp \
           ../../corelib/global/qglobal.cpp \
           ../../corelib/global/qlogging.cpp \
           ../../corelib/global/qmalloc.cpp \
           ../../corelib/global/qnumeric.cpp \
           ../../corelib/global/qoperatingsystemversion.cpp \
           ../../corelib/global/qrandom.cpp \
           ../../corelib/io/qabstractfileengine.cpp \
           ../../corelib/io/qbuffer.cpp \
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
           ../../corelib/io/qipaddress.cpp \
           ../../corelib/io/qfiledevice.cpp \
           ../../corelib/io/qresource.cpp \
           ../../corelib/io/qtemporarydir.cpp \
           ../../corelib/io/qtemporaryfile.cpp \
           ../../corelib/io/qsavefile.cpp \
           ../../corelib/io/qstandardpaths.cpp \
           ../../corelib/io/qloggingcategory.cpp \
           ../../corelib/io/qloggingregistry.cpp \
           ../../corelib/io/qurl.cpp \
           ../../corelib/io/qurlidna.cpp \
           ../../corelib/io/qurlquery.cpp \
           ../../corelib/io/qurlrecode.cpp \
           ../../corelib/kernel/qcoreapplication.cpp \
           ../../corelib/kernel/qcoreglobaldata.cpp \
           ../../corelib/kernel/qmetatype.cpp \
           ../../corelib/kernel/qvariant.cpp \
           ../../corelib/kernel/qsystemerror.cpp \
           ../../corelib/kernel/qsharedmemory.cpp \
           ../../corelib/kernel/qsystemsemaphore.cpp \
           ../../corelib/plugin/quuid.cpp \
           ../../corelib/serialization/qcborcommon.cpp \
           ../../corelib/serialization/qcborstreamwriter.cpp \
           ../../corelib/serialization/qcborvalue.cpp \
           ../../corelib/serialization/qdatastream.cpp \
           ../../corelib/serialization/qjsoncbor.cpp \
           ../../corelib/serialization/qjsondocument.cpp \
           ../../corelib/serialization/qjsonobject.cpp \
           ../../corelib/serialization/qjsonarray.cpp \
           ../../corelib/serialization/qjsonvalue.cpp \
           ../../corelib/serialization/qjsonparser.cpp \
           ../../corelib/serialization/qjsonwriter.cpp \
           ../../corelib/serialization/qtextstream.cpp \
           ../../corelib/serialization/qxmlutils.cpp \
           ../../corelib/serialization/qxmlstream.cpp \
           ../../corelib/text/qbytearray.cpp \
           ../../corelib/text/qbytearraylist.cpp \
           ../../corelib/text/qbytearraymatcher.cpp \
           ../../corelib/text/qlocale.cpp \
           ../../corelib/text/qlocale_tools.cpp \
           ../../corelib/text/qregexp.cpp \
           ../../corelib/text/qstring.cpp \
           ../../corelib/text/qstringbuilder.cpp \
           ../../corelib/text/qstring_compat.cpp \
           ../../corelib/text/qstringlist.cpp \
           ../../corelib/text/qstringview.cpp \
           ../../corelib/text/qvsnprintf.cpp \
           ../../corelib/time/qcalendar.cpp \
           ../../corelib/time/qdatetime.cpp \
           ../../corelib/time/qgregoriancalendar.cpp \
           ../../corelib/time/qromancalendar.cpp \
           ../../corelib/tools/qarraydata.cpp \
           ../../corelib/tools/qbitarray.cpp \
           ../../corelib/tools/qcommandlineparser.cpp \
           ../../corelib/tools/qcommandlineoption.cpp \
           ../../corelib/tools/qcryptographichash.cpp \
           ../../corelib/tools/qhash.cpp \
           ../../corelib/tools/qlist.cpp \
           ../../corelib/tools/qmap.cpp \
           ../../corelib/tools/qringbuffer.cpp \
           ../../corelib/tools/qpoint.cpp \
           ../../corelib/tools/qrect.cpp \
           ../../corelib/tools/qsize.cpp \
           ../../corelib/tools/qline.cpp \
           ../../corelib/tools/qversionnumber.cpp \
           ../../xml/dom/qdom.cpp \
           ../../xml/sax/qxml.cpp

unix:SOURCES += ../../corelib/kernel/qcore_unix.cpp \
                ../../corelib/kernel/qsharedmemory_posix.cpp \
                ../../corelib/kernel/qsharedmemory_systemv.cpp \
                ../../corelib/kernel/qsharedmemory_unix.cpp \
                ../../corelib/kernel/qsystemsemaphore_posix.cpp \
                ../../corelib/kernel/qsystemsemaphore_systemv.cpp \
                ../../corelib/kernel/qsystemsemaphore_unix.cpp \
                ../../corelib/io/qfilesystemengine_unix.cpp \
                ../../corelib/io/qfilesystemiterator_unix.cpp \
                ../../corelib/io/qfsfileengine_unix.cpp

win32:SOURCES += ../../corelib/global/qoperatingsystemversion_win.cpp \
                 ../../corelib/io/qfilesystemengine_win.cpp \
                 ../../corelib/io/qfilesystemiterator_win.cpp \
                 ../../corelib/io/qfsfileengine_win.cpp \
                 ../../corelib/kernel/qcoreapplication_win.cpp \
                 ../../corelib/kernel/qsharedmemory_win.cpp \
                 ../../corelib/kernel/qsystemsemaphore_win.cpp \
                 ../../corelib/plugin/qsystemlibrary.cpp \
                 ../../corelib/kernel/qwinregistry.cpp \

mac {
    SOURCES += \
        ../../corelib/kernel/qcoreapplication_mac.cpp \
        ../../corelib/kernel/qcore_mac.mm \
        ../../corelib/global/qoperatingsystemversion_darwin.mm \
        ../../corelib/kernel/qcore_foundation.mm

    LIBS += -framework Foundation
    osx: LIBS_PRIVATE += -framework CoreServices
    uikit: LIBS_PRIVATE += -framework UIKit
}

macx {
    OBJECTIVE_SOURCES += \
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
    LIBS += -luser32 -lole32 -ladvapi32 -lshell32 -lnetapi32
    mingw: LIBS += -luuid
}

load(qt_module)

CONFIG -= create_cmake

lib.CONFIG = dummy_install
INSTALLS += lib
