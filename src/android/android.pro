CONFIG += java
TARGET = QtAndroid
DESTDIR = $$[QT_INSTALL_PREFIX/get]/jar

PATHPREFIX = $$PWD/jar/src/org/qtproject/qt5/android/

JAVACLASSPATH += $$PWD/jar/src/
JAVASOURCES += \
    $$PATHPREFIX/QtActivityDelegate.java \
    $$PATHPREFIX/QtEditText.java \
    $$PATHPREFIX/QtInputConnection.java \
    $$PATHPREFIX/QtLayout.java \
    $$PATHPREFIX/QtNative.java \
    $$PATHPREFIX/QtNativeLibrariesDir.java \
    $$PATHPREFIX/QtSurface.java
