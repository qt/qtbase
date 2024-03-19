QT += core gui widgets
QT += core-private # For Notification permission request

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
