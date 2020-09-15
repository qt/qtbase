TEMPLATE = aux
TARGET = dummy # Avoid a conflict with the existing gradle directory
CONFIG -= qt android_install

gradle_files.files = \
    $$PWD/gradlew \
    $$PWD/gradlew.bat \
    $$PWD/gradle.properties
gradle_dirs.files = \
    $$PWD/gradle

gradle_files.path = $$[QT_INSTALL_PREFIX]/src/3rdparty/gradle
gradle_dirs.path = $${gradle_files.path}

INSTALLS += gradle_files gradle_dirs
!prefix_build:!equals(OUT_PWD, $$PWD) {
    # For COPIES to work, files and directory entries need to be separate objects.
    COPIES += gradle_files gradle_dirs
}

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
