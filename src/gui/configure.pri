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

defineTest(qtConfTest_qpaDefaultPlatform) {
    name =
    !isEmpty(config.input.qpa_default_platform): name = $$config.input.qpa_default_platform
    else: !isEmpty(QT_QPA_DEFAULT_PLATFORM): name = $$QT_QPA_DEFAULT_PLATFORM
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
