CONFIG -= qt android_install

gradle.files = \
    $$PWD/gradlew \
    $$PWD/gradlew.bat \
    $$PWD/gradle

gradle.path = $$[QT_INSTALL_PREFIX]/src/3rdparty/gradle

INSTALLS += gradle

!prefix_build:!equals(OUT_PWD, $$PWD) {
    RETURN = $$escape_expand(\\n\\t)
    equals(QMAKE_HOST.os, Windows) {
        RETURN = $$escape_expand(\\r\\n\\t)
    }
    OUT_PATH = $$shell_path($$OUT_PWD)

    QMAKE_POST_LINK += \
        $${QMAKE_COPY} $$shell_path($$PWD/gradlew) $$OUT_PATH $$RETURN \
        $${QMAKE_COPY} $$shell_path($$PWD/gradlew.bat) $$OUT_PATH $$RETURN \
        $${QMAKE_COPY_DIR} $$shell_path($$PWD/gradle) $$OUT_PATH
}
