CONFIG -= qt

javaresources.files = \
    $$PWD/AndroidManifest.xml \
    $$PWD/version.xml \
    $$PWD/res \
    $$PWD/src

javaresources.path = $$[QT_INSTALL_PREFIX]/src/android/java

INSTALLS += javaresources
