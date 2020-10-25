TEMPLATE = aux
TARGET = dummy

CONFIG += single_arch
CONFIG -= qt android_install

templates.files = \
    $$PWD/AndroidManifest.xml \
    $$PWD/build.gradle

templates_dirs.files += $$PWD/res

templates.path = $$[QT_INSTALL_PREFIX]/src/android/templates
templates_dirs.path = $${templates.path}

INSTALLS += templates templates_dirs

!prefix_build:!equals(OUT_PWD, $$PWD) {
    COPIES += templates templates_dirs

    RETURN = $$escape_expand(\\n\\t)
    equals(QMAKE_HOST.os, Windows) {
        RETURN = $$escape_expand(\\r\\n\\t)
    }
    OUT_PATH = $$shell_path($$OUT_PWD)

    QMAKE_POST_LINK += \
        $${QMAKE_COPY} $$shell_path($$PWD/AndroidManifest.xml) $$OUT_PATH $$RETURN \
        $${QMAKE_COPY} $$shell_path($$PWD/build.gradle) $$OUT_PATH $$RETURN \
        $${QMAKE_COPY_DIR} $$shell_path($$PWD/res) $$OUT_PATH
}
