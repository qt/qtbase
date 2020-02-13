# Qt core io module

HEADERS +=  \
        io/qabstractfileengine_p.h \
        io/qbuffer.h \
        io/qdataurl_p.h \
        io/qdebug.h \
        io/qdebug_p.h \
        io/qdir.h \
        io/qdir_p.h \
        io/qdiriterator.h \
        io/qfile.h \
        io/qfiledevice.h \
        io/qfiledevice_p.h \
        io/qfileinfo.h \
        io/qfileinfo_p.h \
        io/qipaddress_p.h \
        io/qiodevice.h \
        io/qiodevice_p.h \
        io/qlockfile.h \
        io/qlockfile_p.h \
        io/qnoncontiguousbytedevice_p.h \
        io/qtemporarydir.h \
        io/qtemporaryfile.h \
        io/qtemporaryfile_p.h \
        io/qresource_p.h \
        io/qresource_iterator_p.h \
        io/qsavefile.h \
        io/qstandardpaths.h \
        io/qstorageinfo.h \
        io/qstorageinfo_p.h \
        io/qurl.h \
        io/qurl_p.h \
        io/qurlquery.h \
        io/qurltlds_p.h \
        io/qtldurl_p.h \
        io/qfsfileengine_p.h \
        io/qfsfileengine_iterator_p.h \
        io/qfilesystementry_p.h \
        io/qfilesystemengine_p.h \
        io/qfilesystemmetadata_p.h \
        io/qfilesystemiterator_p.h \
        io/qfileselector.h \
        io/qfileselector_p.h \
        io/qloggingcategory.h \
        io/qloggingregistry_p.h

SOURCES += \
        io/qabstractfileengine.cpp \
        io/qbuffer.cpp \
        io/qdataurl.cpp \
        io/qtldurl.cpp \
        io/qdebug.cpp \
        io/qdir.cpp \
        io/qdiriterator.cpp \
        io/qfile.cpp \
        io/qfiledevice.cpp \
        io/qfileinfo.cpp \
        io/qipaddress.cpp \
        io/qiodevice.cpp \
        io/qlockfile.cpp \
        io/qnoncontiguousbytedevice.cpp \
        io/qstorageinfo.cpp \
        io/qtemporarydir.cpp \
        io/qtemporaryfile.cpp \
        io/qresource.cpp \
        io/qresource_iterator.cpp \
        io/qsavefile.cpp \
        io/qstandardpaths.cpp \
        io/qurl.cpp \
        io/qurlidna.cpp \
        io/qurlquery.cpp \
        io/qurlrecode.cpp \
        io/qfsfileengine.cpp \
        io/qfsfileengine_iterator.cpp \
        io/qfilesystementry.cpp \
        io/qfilesystemengine.cpp \
        io/qfileselector.cpp \
        io/qloggingcategory.cpp \
        io/qloggingregistry.cpp

qtConfig(zstd): QMAKE_USE_PRIVATE += zstd

qtConfig(filesystemwatcher) {
    HEADERS += \
        io/qfilesystemwatcher.h \
        io/qfilesystemwatcher_p.h \
        io/qfilesystemwatcher_polling_p.h
    SOURCES += \
        io/qfilesystemwatcher.cpp \
        io/qfilesystemwatcher_polling.cpp

    win32 {
        SOURCES += io/qfilesystemwatcher_win.cpp
        HEADERS += io/qfilesystemwatcher_win_p.h
    } else:macos {
        OBJECTIVE_SOURCES += io/qfilesystemwatcher_fsevents.mm
        HEADERS += io/qfilesystemwatcher_fsevents_p.h
    } else:qtConfig(inotify) {
        SOURCES += io/qfilesystemwatcher_inotify.cpp
        HEADERS += io/qfilesystemwatcher_inotify_p.h
    } else {
        freebsd|darwin|openbsd|netbsd {
            SOURCES += io/qfilesystemwatcher_kqueue.cpp
            HEADERS += io/qfilesystemwatcher_kqueue_p.h
        }
    }
}

qtConfig(processenvironment) {
    SOURCES += \
        io/qprocess.cpp
    HEADERS += \
        io/qprocess.h \
        io/qprocess_p.h

    win32:!winrt: \
        SOURCES += io/qprocess_win.cpp
    else: unix: \
        SOURCES += io/qprocess_unix.cpp
}

qtConfig(settings) {
    SOURCES += \
        io/qsettings.cpp
    HEADERS += \
        io/qsettings.h \
        io/qsettings_p.h

    win32 {
        !winrt {
            SOURCES += io/qsettings_win.cpp
        } else {
            SOURCES += io/qsettings_winrt.cpp
        }
    } else: darwin:!nacl {
        SOURCES += io/qsettings_mac.cpp
    }
    wasm : SOURCES += io/qsettings_wasm.cpp
}

win32 {
        SOURCES += io/qfsfileengine_win.cpp
        SOURCES += io/qlockfile_win.cpp
        SOURCES += io/qfilesystemengine_win.cpp

        qtConfig(filesystemiterator) {
            SOURCES += io/qfilesystemiterator_win.cpp
        }

    !winrt {
        HEADERS += \
            io/qwindowspipereader_p.h \
            io/qwindowspipewriter_p.h

        SOURCES += \
            io/qstandardpaths_win.cpp \
            io/qstorageinfo_win.cpp \
            io/qwindowspipereader.cpp \
            io/qwindowspipewriter.cpp

        LIBS += -lmpr -luserenv
        QMAKE_USE_PRIVATE += netapi32
    } else {
        SOURCES += \
                io/qstandardpaths_winrt.cpp \
                io/qstorageinfo_stub.cpp
    }
} else:unix {
        SOURCES += \
                io/qfsfileengine_unix.cpp \
                io/qfilesystemengine_unix.cpp \
                io/qlockfile_unix.cpp \
                io/qfilesystemiterator_unix.cpp

        !integrity:!uikit:!rtems {
            SOURCES += io/forkfd_qt.cpp
            HEADERS += \
                     ../3rdparty/forkfd/forkfd.h
            INCLUDEPATH += ../3rdparty/forkfd
        }
        mac {
            SOURCES += io/qstorageinfo_mac.cpp
            qtConfig(processenvironment): \
                OBJECTIVE_SOURCES += io/qprocess_darwin.mm
            OBJECTIVE_SOURCES += \
                io/qstandardpaths_mac.mm \
                io/qfilesystemengine_mac.mm
            osx {
                LIBS += -framework DiskArbitration -framework IOKit
            } else {
                LIBS += -framework MobileCoreServices
            }
        } else:android:!android-embedded {
            SOURCES += \
                io/qstandardpaths_android.cpp \
                io/qstorageinfo_unix.cpp
        } else:haiku {
            SOURCES += \
                io/qstandardpaths_haiku.cpp \
                io/qstorageinfo_unix.cpp
            LIBS += -lbe
        } else {
            SOURCES += \
                io/qstandardpaths_unix.cpp \
                io/qstorageinfo_unix.cpp
        }
}

