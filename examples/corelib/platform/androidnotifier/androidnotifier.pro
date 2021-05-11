QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += \
    main.cpp \
    notificationclient.cpp

HEADERS += \
    notificationclient.h

RESOURCES += \
    main.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/corelib/platform/androidnotifier
INSTALLS += target

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
OTHER_FILES += \
    android/src/org/qtproject/example/androidnotifier/NotificationClient.java \
    android/AndroidManifest.xml
