TEMPLATE = lib
TARGET = QtUiTools
QT += xml
CONFIG += qt staticlib
DESTDIR = ../../../../lib
DLLDESTDIR = ../../../../bin

symbian {
    TARGET.UID3 = 0x2001E628
    load(armcc_warnings)
}

win32|mac:!macx-xcode:CONFIG += debug_and_release build_all

DEFINES += QFORMINTERNAL_NAMESPACE QT_DESIGNER_STATIC QT_FORMBUILDER_NO_SCRIPT
isEmpty(QT_MAJOR_VERSION) {
   VERSION=4.3.0
} else {
   VERSION=$${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}
}
include(../../../../src/qt_targets.pri)
QMAKE_TARGET_PRODUCT = UiLoader
QMAKE_TARGET_DESCRIPTION = QUiLoader

include(../lib/uilib/uilib.pri)

HEADERS += quiloader.h
SOURCES += quiloader.cpp

include($$QT_BUILD_TREE/include/QtUiTools/headers.pri, "", true)
quitools_headers.files = $$SYNCQT.HEADER_FILES $$SYNCQT.HEADER_CLASSES
quitools_headers.path = $$[QT_INSTALL_HEADERS]/QtUiTools
INSTALLS        += quitools_headers

target.path=$$[QT_INSTALL_LIBS]
INSTALLS        += target

unix|win32-g++* {
   CONFIG     += create_pc
   QMAKE_PKGCONFIG_LIBDIR = $$[QT_INSTALL_LIBS]
   QMAKE_PKGCONFIG_INCDIR = $$[QT_INSTALL_HEADERS]/$$TARGET
   QMAKE_PKGCONFIG_CFLAGS = -I$$[QT_INSTALL_HEADERS]
   QMAKE_PKGCONFIG_DESTDIR = pkgconfig
   QMAKE_PKGCONFIG_REQUIRES += QtXml
}

TARGET = $$qtLibraryTarget($$TARGET$$QT_LIBINFIX) #do this towards the end
