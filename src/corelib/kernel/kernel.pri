# Qt core object module

HEADERS += \
        kernel/qabstracteventdispatcher.h \
        kernel/qabstractnativeeventfilter.h \
        kernel/qbasictimer.h \
        kernel/qdeadlinetimer.h \
        kernel/qdeadlinetimer_p.h \
        kernel/qelapsedtimer.h \
        kernel/qeventloop.h \
        kernel/qpointer.h \
        kernel/qcorecmdlineargs_p.h \
        kernel/qcoreapplication.h \
        kernel/qcoreevent.h \
        kernel/qmetaobject.h \
        kernel/qmetatype.h \
        kernel/qmimedata.h \
        kernel/qobject.h \
        kernel/qobject_impl.h \
        kernel/qobjectdefs.h \
        kernel/qobjectdefs_impl.h \
        kernel/qsignalmapper.h \
        kernel/qsocketnotifier.h \
        kernel/qtimer.h \
        kernel/qtranslator.h \
        kernel/qtranslator_p.h \
        kernel/qvariant.h \
        kernel/qabstracteventdispatcher_p.h \
        kernel/qcoreapplication_p.h \
        kernel/qobjectcleanuphandler.h \
        kernel/qvariant_p.h \
        kernel/qmetaobject_p.h \
        kernel/qmetaobject_moc_p.h \
        kernel/qmetaobjectbuilder_p.h \
        kernel/qobject_p.h \
        kernel/qcoreglobaldata_p.h \
        kernel/qsharedmemory.h \
        kernel/qsharedmemory_p.h \
        kernel/qsystemsemaphore.h \
        kernel/qsystemsemaphore_p.h \
        kernel/qfunctions_p.h \
        kernel/qmath.h \
        kernel/qsystemerror_p.h \
        kernel/qmetatype_p.h \
        kernel/qmetatypeswitcher_p.h \
        kernel/qtestsupport_core.h

SOURCES += \
        kernel/qabstracteventdispatcher.cpp \
        kernel/qabstractnativeeventfilter.cpp \
        kernel/qbasictimer.cpp \
        kernel/qdeadlinetimer.cpp \
        kernel/qelapsedtimer.cpp \
        kernel/qeventloop.cpp \
        kernel/qcoreapplication.cpp \
        kernel/qcoreevent.cpp \
        kernel/qmetaobject.cpp \
        kernel/qmetatype.cpp \
        kernel/qmetaobjectbuilder.cpp \
        kernel/qmimedata.cpp \
        kernel/qobject.cpp \
        kernel/qobjectcleanuphandler.cpp \
        kernel/qsignalmapper.cpp \
        kernel/qsocketnotifier.cpp \
        kernel/qtimer.cpp \
        kernel/qtranslator.cpp \
        kernel/qvariant.cpp \
        kernel/qcoreglobaldata.cpp \
        kernel/qsharedmemory.cpp \
        kernel/qsystemsemaphore.cpp \
        kernel/qpointer.cpp \
        kernel/qmath.cpp \
        kernel/qsystemerror.cpp \
        kernel/qtestsupport_core.cpp

win32 {
        SOURCES += \
                kernel/qcoreapplication_win.cpp \
                kernel/qelapsedtimer_win.cpp \
                kernel/qwineventnotifier.cpp \
                kernel/qsharedmemory_win.cpp \
                kernel/qsystemsemaphore_win.cpp
        HEADERS += \
                kernel/qwineventnotifier.h \
                kernel/qwineventnotifier_p.h

        winrt {
            SOURCES += kernel/qeventdispatcher_winrt.cpp
            HEADERS += kernel/qeventdispatcher_winrt_p.h
        } else {
            SOURCES += kernel/qeventdispatcher_win.cpp \
                       kernel/qwinregistry.cpp
            HEADERS += kernel/qeventdispatcher_win_p.h \
                       kernel/qwinregistry_p.h
        }

        !winrt: LIBS_PRIVATE += -lversion
}

winrt {
        SOURCES += \
                kernel/qfunctions_winrt.cpp
        HEADERS += \
                kernel/qfunctions_fake_env_p.h \
                kernel/qfunctions_winrt.h
}

mac {
    HEADERS += \
        kernel/qcfsocketnotifier_p.h \
        kernel/qcore_mac_p.h \
        kernel/qeventdispatcher_cf_p.h

    SOURCES += \
        kernel/qcfsocketnotifier.cpp \
        kernel/qcoreapplication_mac.cpp \
        kernel/qcore_foundation.mm \
        kernel/qcore_mac.mm \
        kernel/qeventdispatcher_cf.mm

    !nacl: SOURCES += kernel/qelapsedtimer_mac.cpp

    LIBS_PRIVATE += -framework Foundation

    osx: LIBS_PRIVATE += -framework CoreServices -framework AppKit -framework Security

    ios|tvos {
        # We need UIKit for UIApplication in qeventdispatcher_cf.mm
        LIBS_PRIVATE += -framework UIKit
    }

    watchos {
        # We need WatchKit for WKExtension in qeventdispatcher_cf.mm
        LIBS_PRIVATE += -framework WatchKit
    }
}

nacl {
    SOURCES += \
        kernel/qfunctions_nacl.cpp
    HEADERS += \
        kernel/qfunctions_nacl.h
}

unix|integrity {
    SOURCES += \
            kernel/qcore_unix.cpp \
            kernel/qeventdispatcher_unix.cpp \
            kernel/qtimerinfo_unix.cpp
    !darwin|nacl: SOURCES += kernel/qelapsedtimer_unix.cpp

    HEADERS += \
            kernel/qcore_unix_p.h \
            kernel/qeventdispatcher_unix_p.h \
            kernel/qpoll_p.h \
            kernel/qtimerinfo_unix_p.h

    qtConfig(poll_select): SOURCES += kernel/qpoll.cpp

    qtConfig(glib) {
        SOURCES += \
            kernel/qeventdispatcher_glib.cpp
        HEADERS += \
            kernel/qeventdispatcher_glib_p.h
        QMAKE_USE_PRIVATE += glib
    }

    qtConfig(clock-gettime): QMAKE_USE_PRIVATE += librt

    !android {
        SOURCES += kernel/qsharedmemory_posix.cpp \
                   kernel/qsharedmemory_systemv.cpp \
                   kernel/qsharedmemory_unix.cpp \
                   kernel/qsystemsemaphore_posix.cpp \
                   kernel/qsystemsemaphore_systemv.cpp \
                   kernel/qsystemsemaphore_unix.cpp
    } else {
        SOURCES += kernel/qsharedmemory_android.cpp \
                   kernel/qsystemsemaphore_android.cpp
    }

    # This is needed by QMetaType::typeName array implementation
    integrity: QMAKE_CXXFLAGS += --pending_instantiations=128
}

vxworks {
        SOURCES += \
                kernel/qfunctions_vxworks.cpp
        HEADERS += \
                kernel/qfunctions_vxworks.h
}

qnx:qtConfig(qqnx_pps) {
        QMAKE_USE_PRIVATE += pps
        SOURCES += \
                kernel/qppsattribute.cpp \
                kernel/qppsobject.cpp
        HEADERS += \
                kernel/qppsattribute_p.h \
                kernel/qppsattributeprivate_p.h \
                kernel/qppsobject_p.h \
                kernel/qppsobjectprivate_p.h
}

android:!android-embedded {
        SOURCES += \
                   kernel/qjnionload.cpp \
                   kernel/qjnihelpers.cpp \
                   kernel/qjni.cpp
        HEADERS += \
                   kernel/qjnihelpers_p.h \
                   kernel/qjni_p.h
}

!darwin:!unix:!win32: SOURCES += kernel/qelapsedtimer_generic.cpp
