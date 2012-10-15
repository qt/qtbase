option(host_build)
TEMPLATE = lib
TARGET = bootstrap
CONFIG += static

CONFIG += console qtinc 
CONFIG -= qt
CONFIG += exceptions_off
!build_pass:contains(QT_CONFIG, build_all):CONFIG += release
mac:CONFIG -= app_bundle incremental

DEFINES += \
        QT_BOOTSTRAPPED \
        QT_LITE_UNICODE \
        QT_NO_CAST_FROM_ASCII \
        QT_NO_CAST_TO_ASCII \
        QT_NO_CODECS \
        QT_NO_DATASTREAM \
        QT_NO_LIBRARY \
        QT_NO_QOBJECT \
        QT_NO_SYSTEMLOCALE \
        QT_NO_THREAD \
        QT_NO_UNICODETABLES \
        QT_NO_USING_NAMESPACE \
        QT_NO_DEPRECATED \
        QT_QMAKE_LOCATION=\\\"$$QMAKE_QMAKE\\\"

INCLUDEPATH += $$QT_BUILD_TREE/include \
            $$QT_BUILD_TREE/include/QtCore \
            $$QT_BUILD_TREE/include/QtCore/$$QT_VERSION \
            $$QT_BUILD_TREE/include/QtCore/$$QT_VERSION/QtCore \
            $$QT_BUILD_TREE/src/corelib/global

DEPENDPATH += $$INCLUDEPATH \
              ../../corelib/global \
              ../../corelib/kernel \
              ../../corelib/tools \
              ../../corelib/io \
              ../../corelib/codecs \
              ../../corelib/json

SOURCES += \
           ../../corelib/codecs/qlatincodec.cpp \
           ../../corelib/codecs/qtextcodec.cpp \
           ../../corelib/codecs/qutfcodec.cpp \
           ../../corelib/global/qglobal.cpp \
           ../../corelib/global/qlibraryinfo.cpp \
           ../../corelib/global/qlogging.cpp \
           ../../corelib/global/qmalloc.cpp \
           ../../corelib/global/qnumeric.cpp \
           ../../corelib/io/qabstractfileengine.cpp \
           ../../corelib/io/qbuffer.cpp \
           ../../corelib/io/qdatastream.cpp \
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
           ../../corelib/io/qsettings.cpp \
           ../../corelib/io/qtemporaryfile.cpp \
           ../../corelib/io/qtextstream.cpp \
           ../../corelib/kernel/qcoreglobaldata.cpp \
           ../../corelib/kernel/qmetatype.cpp \
           ../../corelib/kernel/qvariant.cpp \
           ../../corelib/kernel/qsystemerror.cpp \
           ../../corelib/plugin/quuid.cpp \
           ../../corelib/tools/qbitarray.cpp \
           ../../corelib/tools/qbytearray.cpp \
           ../../corelib/tools/qarraydata.cpp \
           ../../corelib/tools/qbytearraymatcher.cpp \
           ../../corelib/tools/qdatetime.cpp \
           ../../corelib/tools/qhash.cpp \
           ../../corelib/tools/qlist.cpp \
           ../../corelib/tools/qlocale.cpp \
           ../../corelib/tools/qlocale_tools.cpp \
           ../../corelib/tools/qmap.cpp \
           ../../corelib/tools/qregexp.cpp \
           ../../corelib/tools/qpoint.cpp \
           ../../corelib/tools/qrect.cpp \
           ../../corelib/tools/qsize.cpp \
           ../../corelib/tools/qline.cpp \
           ../../corelib/tools/qstring.cpp \
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
           ../../corelib/json/qjsonwriter.cpp

unix:SOURCES += ../../corelib/io/qfilesystemengine_unix.cpp \
                ../../corelib/io/qfilesystemiterator_unix.cpp \
                ../../corelib/io/qfsfileengine_unix.cpp

win32:SOURCES += ../../corelib/io/qfilesystemengine_win.cpp \
                 ../../corelib/io/qfilesystemiterator_win.cpp \
                 ../../corelib/io/qfsfileengine_win.cpp \
                 ../../corelib/io/qsettings_win.cpp \
                 ../../corelib/plugin/qsystemlibrary.cpp \

macx: {
   SOURCES += ../../corelib/io/qfilesystemengine_mac.cpp \
              ../../corelib/io/qsettings_mac.cpp \
              ../../corelib/kernel/qcore_mac.cpp
   LIBS += -framework CoreServices
}

if(contains(QT_CONFIG, zlib)|cross_compile):include(../../3rdparty/zlib.pri)
else:include(../../3rdparty/zlib_dependency.pri)

lib.CONFIG = dummy_install
INSTALLS += lib

# Make dummy "sis" and "freeze" target to keep recursive "make sis/freeze" working.
sis_target.target = sis
sis_target.commands =
sis_target.depends = first
QMAKE_EXTRA_TARGETS += sis_target
freeze_target.target = freeze
freeze_target.commands =
freeze_target.depends = first
QMAKE_EXTRA_TARGETS += freeze_target

# We need the forwarding headers before their respective modules are built,
# so do a minimal syncqt run.
qtPrepareTool(QMAKE_SYNCQT, syncqt)
QTDIR = $$[QT_HOST_PREFIX]
exists($$QTDIR/.qmake.cache): \
    mod_component_base = $$QTDIR
else: \
    mod_component_base = $$dirname(_QMAKE_CACHE_)
QMAKE_SYNCQT += -minimal -module QtCore -module QtDBus -module QtXml \
    -mkspecsdir $$[QT_HOST_DATA/get]/mkspecs -outdir $$mod_component_base $$dirname(_QMAKE_CONF_)
!silent:message($$QMAKE_SYNCQT)
system($$QMAKE_SYNCQT)|error("Failed to run: $$QMAKE_SYNCQT")
