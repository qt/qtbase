# Qt core mimetype module

qtConfig(mimetype) {
    HEADERS += \
        mimetypes/qmimedatabase.h \
        mimetypes/qmimetype.h \
        mimetypes/qmimemagicrulematcher_p.h \
        mimetypes/qmimetype_p.h \
        mimetypes/qmimetypeparser_p.h \
        mimetypes/qmimedatabase_p.h \
        mimetypes/qmimemagicrule_p.h \
        mimetypes/qmimeglobpattern_p.h \
        mimetypes/qmimeprovider_p.h

    SOURCES += \
        mimetypes/qmimedatabase.cpp \
        mimetypes/qmimetype.cpp \
        mimetypes/qmimemagicrulematcher.cpp \
        mimetypes/qmimetypeparser.cpp \
        mimetypes/qmimemagicrule.cpp \
        mimetypes/qmimeglobpattern.cpp \
        mimetypes/qmimeprovider.cpp

    MIME_DATABASE = mimetypes/mime/packages/freedesktop.org.xml
    OTHER_FILES += $$MIME_DATABASE

    qtConfig(mimetype-database) {
        outpath = .rcc
        android {
            outpath = $$outpath/$${QT_ARCH}
        }
        debug_and_release {
            CONFIG(debug, debug|release): outpath = $$outpath/debug
            else:                         outpath = $$outpath/release
        }

        mimedb.depends = $$PWD/mime/generate.pl
        equals(MAKEFILE_GENERATOR, MSVC.NET)|equals(MAKEFILE_GENERATOR, MSBUILD)|isEmpty(QMAKE_SH) {
            mimedb.commands = cmd /c $$shell_path($$PWD/mime/generate.bat)
            mimedb.depends += $$PWD/mime/generate.bat $$PWD/mime/hexdump.ps1
        } else {
            mimedb.commands = perl $${mimedb.depends}
        }

        qtConfig(zstd): mimedb.commands += --zstd
        mimedb.commands += ${QMAKE_FILE_IN} > ${QMAKE_FILE_OUT}

        mimedb.output = $$outpath/qmimeprovider_database.cpp
        mimedb.input = MIME_DATABASE
        mimedb.variable_out = INCLUDED_SOURCES
        QMAKE_EXTRA_COMPILERS += mimedb
        INCLUDEPATH += $$outpath
        unset(outpath)
    }
}
