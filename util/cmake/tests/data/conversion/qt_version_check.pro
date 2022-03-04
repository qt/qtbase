QT += core gui
SOURCES += main.cpp
greaterThan(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 1):equals(QT_PATCH_VERSION, 0) {
    DEFINES += SUPER_FRESH_MAJOR_QT_RELEASE
}
greaterThan(QT_VERSION, 6.6.5):lessThan(QT_VERSION, 6.6.7):equals(QT_VERSION, 6.6.6): {
    DEFINES += QT_VERSION_OF_THE_BEAST
}
