CONFIG += single_arch

TARGET = QtAndroidNetwork

load(qt_build_paths)
CONFIG += java
DESTDIR = $$MODULE_BASE_OUTDIR/jar

JAVACLASSPATH += $$PWD/src

JAVASOURCES += $$PWD/src/org/qtproject/qt5/android/network/QtNetwork.java

# install
target.path = $$[QT_INSTALL_PREFIX]/jar
INSTALLS += target
