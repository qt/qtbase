TEMPLATE = subdirs
TARGET = sqldatabase_snippets
SUBDIRS =

contains(QT_BUILD_PARTS, tests) {
    SUBDIRS += \
        code \
        sqldatabase
}

