# custom tests

defineTest(qtConfLibrary_freetype) {
    input = $$eval($${2}.alias)
    isEmpty(config.input.$${input}.incdir) {
        TRY_INCLUDEPATHS = $$EXTRA_INCLUDEPATH $$QMAKE_INCDIR_X11
        haiku: TRY_INCLUDEPATHS += /system/develop/headers
        TRY_INCLUDEPATHS += $$QMAKE_DEFAULT_INCDIRS
        for (p, TRY_INCLUDEPATHS) {
            includedir = $$p/freetype2
            exists($$includedir) {
                config.input.$${input}.incdir = $$includedir
                export(config.input.$${input}.incdir)
                break()
            }
        }
    }
    qtConfLibrary_inline($$1, $$2): return(true)
    return(false)
}

# Check for Direct X shader compiler 'fxc'.
# Up to Direct X SDK June 2010 and for MinGW, this is pointed to by the
# DXSDK_DIR variable. Starting with Windows Kit 8, it is included in
# the Windows SDK.
defineTest(qtConfTest_fxc) {
    !mingw {
        fxc = $$qtConfFindInPath("fxc.exe")
    } else {
        equals(QMAKE_HOST.arch, x86_64): \
            fns = x64/fxc.exe
        else: \
            fns = x86/fxc.exe
        dxdir = $$(DXSDK_DIR)
        !isEmpty(dxdir) {
            fxc = $$dxdir/Utilities/bin/$$fns
        } else {
            winkitbindir = $$(WindowsSdkVerBinPath)
            !isEmpty(winkitbindir) {
                fxc = $$winkitbindir/$$fns
            } else {
                winkitdir = $$(WindowsSdkDir)
                !isEmpty(winkitdir): \
                    fxc = $$winkitdir/bin/$$fns
            }
        }
    }

    !isEmpty(fxc):exists($$fxc) {
        $${1}.value = $$clean_path($$fxc)
        export($${1}.value)
        $${1}.cache += value
        export($${1}.cache)
        return(true)
    }
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
    else: wasm: name = wasm
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
