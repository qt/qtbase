# custom tests

defineTest(qtConfLibrary_freetype) {
    TRY_INCLUDEPATHS = $$EXTRA_INCLUDEPATH $$QMAKE_INCDIR_X11
    haiku: TRY_INCLUDEPATHS += /system/develop/headers
    TRY_INCLUDEPATHS += $$QMAKE_DEFAULT_INCDIR
    for (p, TRY_INCLUDEPATHS) {
        includedir = $$p/freetype2
        exists($$includedir) {
            $${1}.includedir = "$$val_escape(includedir)"
            export($${1}.includedir)
            return(true)
        }
    }
    return(true)
}

# Check for Direct X SDK (include, lib, and direct shader compiler 'fxc').
# Up to Direct X SDK June 2010 and for MinGW, this is pointed to by the
# DXSDK_DIR variable. Starting with Windows Kit 8, it is included in
# the Windows SDK. Checking for the header is not sufficient, since it
# is also present in MinGW.
defineTest(qtConfTest_directX) {
    dxdir = $$getenv("DXSDK_DIR")
    !isEmpty(dxdir) {
        EXTRA_INCLUDEPATH += $$dxdir/include
        equals(QT_ARCH, x86_64): \
            EXTRA_LIBDIR += $$dxdir/lib/x64
        else: \
            EXTRA_LIBDIR += $$dxdir/lib/x86
        EXTRA_PATH += $$dxdir/Utilities/bin/x86
    }

    $$qtConfEvaluate("features.sse2") {
        ky = $$size($${1}.files._KEYS_)
        $${1}.files._KEYS_ += $$ky
        # Not present on MinGW-32
        $${1}.files.$${ky} = "intrin.h"
    }

    qtConfTest_files($${1}): return(true)
    return(false)
}

defineTest(qtConfTest_qpaDefaultPlatform) {
    name =
    !isEmpty(config.input.qpa_default_platform): name = $$config.input.qpa_default_platform
    else: !isEmpty(QT_QPA_DEFAULT_PLATFORM): name = $$QT_QPA_DEFAULT_PLATFORM
    else: winrt: name = winrt
    else: win32: name = windows
    else: android: name = android
    else: macos: name = cocoa
    else: if(ios|tvos): name = ios
    else: watchos: name = minimal
    else: qnx: name = qnx
    else: integrity: name = integrityfb
    else: haiku: name = haiku
    else: name = xcb

    $${1}.value = $$name
    $${1}.plugin = q$$name
    $${1}.name = "\"$$name\""
    export($${1}.value)
    export($${1}.plugin)
    export($${1}.name)
    $${1}.cache += value plugin name
    export($${1}.cache)
    return(true)
}
