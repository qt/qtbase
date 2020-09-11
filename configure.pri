# this must be done outside any function
QT_SOURCE_TREE = $$PWD
QT_BUILD_TREE = $$shadowed($$PWD)

# custom command line handling

defineTest(qtConfCommandline_qmakeArgs) {
    contains(1, QMAKE_[A-Z0-9_]+ *[-+]?=.*) {
        config.input.qmakeArgs += $$1
        export(config.input.qmakeArgs)
        return(true)
    }
    return(false)
}

defineTest(qtConfCommandline_cxxstd) {
    arg = $${1}
    val = $${2}
    isEmpty(val): val = $$qtConfGetNextCommandlineArg()
    !contains(val, "^-.*"):!isEmpty(val) {
        contains(val, "(c\+\+)?11") {
            qtConfCommandlineSetInput("c++14", "no")
            qtConfCommandlineSetInput("c++1z", "no")
            qtConfCommandlineSetInput("c++2a", "no")
        } else: contains(val, "(c\+\+)?(14|1y)") {
            qtConfCommandlineSetInput("c++14", "yes")
            qtConfCommandlineSetInput("c++1z", "no")
            qtConfCommandlineSetInput("c++2a", "no")
        } else: contains(val, "(c\+\+)?(17|1z)") {
            qtConfCommandlineSetInput("c++14", "yes")
            qtConfCommandlineSetInput("c++1z", "yes")
            qtConfCommandlineSetInput("c++2a", "no")
        } else: contains(val, "(c\+\+)?(2a)") {
            qtConfCommandlineSetInput("c++14", "yes")
            qtConfCommandlineSetInput("c++1z", "yes")
            qtConfCommandlineSetInput("c++2a", "yes")
        } else {
            qtConfAddError("Invalid argument $$val to command line parameter $$arg")
        }
    } else {
        qtConfAddError("Missing argument to command line parameter $$arg")
    }
}

defineTest(qtConfCommandline_sanitize) {
    arg = $${1}
    val = $${2}
    isEmpty(val): val = $$qtConfGetNextCommandlineArg()
    !contains(val, "^-.*"):!isEmpty(val) {
        equals(val, "address") {
            qtConfCommandlineSetInput("sanitize_address", "yes")
        } else: equals(val, "thread") {
            qtConfCommandlineSetInput("sanitize_thread", "yes")
        } else: equals(val, "memory") {
            qtConfCommandlineSetInput("sanitize_memory", "yes")
        } else: equals(val, "fuzzer-no-link") {
            qtConfCommandlineSetInput("sanitize_fuzzer_no_link", "yes")
        } else: equals(val, "undefined") {
            qtConfCommandlineSetInput("sanitize_undefined", "yes")
        } else {
            qtConfAddError("Invalid argument $$val to command line parameter $$arg")
        }
    } else {
        qtConfAddError("Missing argument to command line parameter $$arg")
    }
}

defineTest(qtConfCommandline_coverage) {
    arg = $${1}
    val = $${2}
    isEmpty(val): val = $$qtConfGetNextCommandlineArg()
    !contains(val, "^-.*"):!isEmpty(val) {
        equals(val, "trace-pc-guard") {
            qtConfCommandlineSetInput("coverage_trace_pc_guard", "yes")
        } else: equals(val, "source-based") {
            qtConfCommandlineSetInput("coverage_source_based", "yes")
        } else: {
            qtConfAddError("Invalid argument $$val to command line parameter $$arg")
        }
    } else {
        qtConfAddError("Missing argument to command line parameter $$arg")
    }
}

# callbacks

defineReplace(qtConfFunc_crossCompile) {
    !isEmpty(config.input.xplatform): return(true)
    !isEmpty(config.input.device-option): return(true)
    !isEmpty(config.input.sysroot): return(true)
    spec = $$[QMAKE_SPEC]
    !equals(spec, $$[QMAKE_XSPEC]): return(true)
    return(false)
}

defineReplace(qtConfFunc_licenseCheck) {
    exists($$QT_SOURCE_TREE/LICENSE.LGPL3)|exists($$QT_SOURCE_TREE/LICENSE.GPL2)|exists($$QT_SOURCE_TREE/LICENSE.GPL3): \
        hasOpenSource = true
    else: \
        hasOpenSource = false
    exists($$QT_SOURCE_TREE/LICENSE.QT-LICENSE-AGREEMENT): \
        hasCommercial = true
    else: \
        hasCommercial = false

    commercial = $$config.input.commercial
    isEmpty(commercial) {
        $$hasOpenSource {
            $$hasCommercial {
                logn()
                logn("Selecting Qt Edition.")
                logn()
                logn("Type 'c' if you want to use the Commercial Edition.")
                logn("Type 'o' if you want to use the Open Source Edition.")
                logn()
                for(ever) {
                    val = $$lower($$prompt("Which edition of Qt do you want to use? ", false))
                    equals(val, c) {
                        commercial = yes
                        QMAKE_SAVED_ARGS += -commercial
                    } else: equals(val, o) {
                        commercial = no
                        QMAKE_SAVED_ARGS += -opensource
                    } else {
                        next()
                    }
                    export(QMAKE_SAVED_ARGS)
                    break()
                }
            } else {
                commercial = no
            }
        } else {
            !$$hasCommercial: \
                qtConfFatalError("No license files. Cannot proceed. Try re-installing Qt.")
            commercial = yes
        }
    }

    equals(commercial, no) {
        !$$hasOpenSource: \
            qtConfFatalError("This is the Qt Commercial Edition." \
                             "Cannot proceed with -opensource.")

        logn()
        logn("This is the Qt Open Source Edition.")

        EditionString = "Open Source"
        config.input.qt_edition = OpenSource
        export(config.input.qt_edition)
    } else {
        !$$hasCommercial: \
            qtConfFatalError("This is the Qt Open Source Edition." \
                             "Cannot proceed with -commercial.")

        !exists($$QT_SOURCE_TREE/.release-timestamp) {
            #  Build from git

            logn()
            logn("This is the Qt Commercial Edition.")

            EditionString = "Commercial"
            config.input.qt_edition = Commercial
            export(config.input.qt_edition)
        } else {
            # Build from a released source package

            equals(QMAKE_HOST.os, Linux) {
                !equals(QMAKE_HOST.arch, x86_64): \
                    Licheck = licheck32
                else: \
                    Licheck = licheck64
            } else: equals(QMAKE_HOST.os, Darwin) {
                Licheck = licheck_mac
            } else: equals(QMAKE_HOST.os, Windows) {
                Licheck = licheck.exe
            } else {
                qtConfFatalError("Host operating system not supported by this edition of Qt.")
            }

            !qtRunLoggedCommand("$$system_quote($$QT_SOURCE_TREE/bin/$$Licheck) \
                                    $$system_quote($$eval(config.input.confirm-license)) \
                                    $$system_quote($$QT_SOURCE_TREE) $$system_quote($$QT_BUILD_TREE) \
                                    $$[QMAKE_SPEC] $$[QMAKE_XSPEC]", \
                                LicheckOutput, false): \
                return(false)
            logn()
            for (o, LicheckOutput) {
                contains(o, "\\w+=.*"): \
                    eval($$o)
                else: \
                    logn($$o)
            }
            config.input.qt_edition = $$Edition
            config.input.qt_licheck = $$Licheck
            config.input.qt_release_date = $$ReleaseDate
            export(config.input.qt_edition)
            export(config.input.qt_licheck)
            export(config.input.qt_release_date)
            return(true)
        }
    }

    !isEmpty(config.input.confirm-license) {
        logn()
        logn("You have already accepted the terms of the $$EditionString license.")
        return(true)
    }

    affix = the
    equals(commercial, no) {
        theLicense = "GNU Lesser General Public License (LGPL) version 3"
        showWhat = "Type 'L' to view the GNU Lesser General Public License version 3 (LGPLv3)."
        gpl2Ok = false
        gpl3Ok = false
        winrt {
            notTheLicense = "Note: GPL version 2 is not available on WinRT."
        } else: wasm {
            gpl3Ok = true
            theLicense = "GNU General Public License (GPL) version 3"
            showWhat = "Type 'G' to view the GNU General Public License version 3 (GPLv3)."
        } else: $$qtConfEvaluate("features.android-style-assets") {
            notTheLicense = "Note: GPL version 2 is not available due to using Android style assets."
        } else {
            theLicense += "or the GNU General Public License (GPL) version 2"
            showWhat += "Type 'G' to view the GNU General Public License version 2 (GPLv2)."
            gpl2Ok = true
            affix = either
        }
    } else {
        theLicense = $$cat($$QT_SOURCE_TREE/LICENSE.QT-LICENSE-AGREEMENT, lines)
        theLicense = $$first(theLicense)
        showWhat = "Type '?' to view the $${theLicense}."
    }
    msg = \
        " " \
        "You are licensed to use this software under the terms of" \
        "the "$$theLicense"." \
        $$notTheLicense \
        " " \
        $$showWhat \
        "Type 'y' to accept this license offer." \
        "Type 'n' to decline this license offer." \
        " "

    for(ever) {
        logn($$join(msg, $$escape_expand(\\n)))
        for(ever) {
            val = $$lower($$prompt("Do you accept the terms of $$affix license? ", false))
            equals(val, y)|equals(val, yes) {
                logn()
                QMAKE_SAVED_ARGS += -confirm-license
                export(QMAKE_SAVED_ARGS)
                return(true)
            } else: equals(val, n)|equals(val, no) {
                return(false)
            } else: equals(commercial, yes):equals(val, ?) {
                licenseFile = $$QT_SOURCE_TREE/LICENSE.QT-LICENSE-AGREEMENT
            } else: equals(commercial, no):equals(val, l) {
                licenseFile = $$QT_SOURCE_TREE/LICENSE.LGPL3
            } else: equals(commercial, no):equals(val, g):$$gpl2Ok {
                licenseFile = $$QT_SOURCE_TREE/LICENSE.GPL2
            } else: equals(commercial, no):equals(val, g):$$gpl3Ok {
                licenseFile = $$QT_SOURCE_TREE/LICENSE.GPL3
            } else {
                next()
            }
            break()
        }
        system("more $$system_quote($$system_path($$licenseFile))")
        logn()
        logn()
    }
}

# custom tests

# this is meant for linux device specs only
defineTest(qtConfTest_machineTuple) {
    qtRunLoggedCommand("$$QMAKE_CXX -dumpmachine", $${1}.tuple)|return(false)
    $${1}.cache += tuple
    export($${1}.cache)
    return(true)
}

defineTest(qtConfTest_verifySpec) {
    qtConfTest_compile($$1): return(true)
    qtConfFatalError("Cannot compile a minimal program. The toolchain or QMakeSpec is broken.", log)
}

defineTest(qtConfTest_architecture) {
    !qtConfTest_compile($${1}): \
        error("Could not determine $$eval($${1}.label). See config.log for details.")

    test = $$eval($${1}.test)
    output = $$eval($${1}.output)
    test_out_dir = $$OUT_PWD/$$basename(QMAKE_CONFIG_TESTS_DIR)/$$test
    test_out_file = $$test_out_dir/$$cat($$test_out_dir/$${output}.target.txt)
    exists($$test_out_file): \
        content = $$cat($$test_out_file, blob)
    else: \
        error("$$eval($${1}.label) detection binary not found.")
    content = $$cat($$test_out_file, blob)

    arch_magic = ".*==Qt=magic=Qt== Architecture:([^\\0]*).*"
    subarch_magic = ".*==Qt=magic=Qt== Sub-architecture:([^\\0]*).*"
    buildabi_magic = ".*==Qt=magic=Qt== Build-ABI:([^\\0]*).*"

    !contains(content, $$arch_magic)|!contains(content, $$subarch_magic)|!contains(content, $$buildabi_magic): \
        error("$$eval($${1}.label) detection binary does not contain expected data.")

    $${1}.arch = $$replace(content, $$arch_magic, "\\1")
    $${1}.subarch = $$replace(content, $$subarch_magic, "\\1")
    $${1}.subarch = $$split($${1}.subarch, " ")
    $${1}.buildabi = $$replace(content, $$buildabi_magic, "\\1")
    export($${1}.arch)
    export($${1}.subarch)
    export($${1}.buildabi)
    qtLog("Detected architecture: $$eval($${1}.arch) ($$eval($${1}.subarch))")

    $${1}.cache += arch subarch buildabi
    export($${1}.cache)
    return(true)
}

defineTest(qtConfTest_gnumake) {
    make = $$qtConfFindInPath("gmake")
    isEmpty(make): make = $$qtConfFindInPath("make")
    !isEmpty(make) {
        qtRunLoggedCommand("$$make -v", version)|return(false)
        contains(version, "^GNU Make.*"): return(true)
    }
    return(false)
}

defineTest(qtConfTest_detectPkgConfig) {
    pkgConfig = $$getenv("PKG_CONFIG")
    !isEmpty(pkgConfig): {
        qtLog("Found pkg-config from environment variable: $$pkgConfig")
    } else {
        pkgConfig = $$QMAKE_PKG_CONFIG

        !isEmpty(pkgConfig) {
            qtLog("Found pkg-config from mkspec: $$pkgConfig")
        } else {
            pkgConfig = $$qtConfFindInPath("pkg-config")

            isEmpty(pkgConfig): \
                return(false)

            qtLog("Found pkg-config from path: $$pkgConfig")
        }
    }

    $$qtConfEvaluate("features.cross_compile") {
        # cross compiling, check that pkg-config is set up sanely
        sysroot = $$config.input.sysroot

        pkgConfigLibdir = $$getenv("PKG_CONFIG_LIBDIR")
        isEmpty(pkgConfigLibdir) {
            isEmpty(sysroot) {
                qtConfAddWarning("Cross compiling without sysroot. Disabling pkg-config")
                return(false)
            }
            !exists("$$sysroot/usr/lib/pkgconfig") {
                qtConfAddWarning( \
                    "Disabling pkg-config since PKG_CONFIG_LIBDIR is not set and" \
                    "the host's .pc files would be used (even if you set PKG_CONFIG_PATH)." \
                    "Set this variable to the directory that contains target .pc files" \
                    "for pkg-config to function correctly when cross-compiling or" \
                    "use -pkg-config to override this test.")
                return(false)
            }

            pkgConfigLibdir = $$sysroot/usr/lib/pkgconfig:$$sysroot/usr/share/pkgconfig
            machineTuple = $$eval($${currentConfig}.tests.machineTuple.tuple)
            !isEmpty(machineTuple): \
                pkgConfigLibdir = "$$pkgConfigLibdir:$$sysroot/usr/lib/$$machineTuple/pkgconfig"

            qtConfAddNote("PKG_CONFIG_LIBDIR automatically set to $$pkgConfigLibdir")
        }
        pkgConfigSysrootDir = $$getenv("PKG_CONFIG_SYSROOT_DIR")
        isEmpty(pkgConfigSysrootDir) {
            isEmpty(sysroot) {
                qtConfAddWarning( \
                    "Disabling pkg-config since PKG_CONFIG_SYSROOT_DIR is not set." \
                    "Set this variable to your sysroot for pkg-config to function correctly when" \
                    "cross-compiling or use -pkg-config to override this test.")
                return(false)
            }

            pkgConfigSysrootDir = $$sysroot
            qtConfAddNote("PKG_CONFIG_SYSROOT_DIR automatically set to $$pkgConfigSysrootDir")
        }
        $${1}.pkgConfigLibdir = $$pkgConfigLibdir
        export($${1}.pkgConfigLibdir)
        $${1}.pkgConfigSysrootDir = $$pkgConfigSysrootDir
        export($${1}.pkgConfigSysrootDir)
        $${1}.cache += pkgConfigLibdir pkgConfigSysrootDir
    }
    $${1}.pkgConfig = $$pkgConfig
    export($${1}.pkgConfig)
    $${1}.cache += pkgConfig
    export($${1}.cache)

    return(true)
}

defineTest(qtConfTest_buildParts) {
    parts = $$config.input.make
    isEmpty(parts) {
        parts = libs examples

        $$qtConfEvaluate("features.developer-build"): \
            parts += tests
        !$$qtConfEvaluate("features.cross_compile"): \
            parts += tools
    }

    parts -= $$config.input.nomake

    # always add libs, as it's required to build Qt
    parts *= libs

    $${1}.value = $$parts
    export($${1}.value)
    $${1}.cache = -
    export($${1}.cache)
    return(true)
}

defineTest(qtConfTest_x86Simd) {
    simd = $$section(1, ".", -1)    # last component
    $${1}.args = CONFIG+=add_cflags DEFINES+=NO_ATTRIBUTE SIMD=$$simd
    $${1}.test = x86_simd
    qtConfTest_compile($${1})
}

defineTest(qtConfTest_x86SimdAlways) {
    configs =
    fpfx = $${currentConfig}.features
    tpfx = $${currentConfig}.tests

    # Make a list of all passing features whose tests have type=x86Simd
    for (f, $${tpfx}._KEYS_) {
        !equals($${tpfx}.$${f}.type, "x86Simd"): \
            next()
        qtConfCheckFeature($$f)
        equals($${fpfx}.$${f}.available, true): configs += $$f
    }
    $${1}.literal_args = SIMD=$$join(configs, " ")
    qtConfTest_compile($${1})
}

# custom outputs

# this reloads the qmakespec as completely as reasonably possible.
defineTest(reloadSpec) {
    bypassNesting() {
        for (f, QMAKE_INTERNAL_INCLUDED_FILES) {
            contains(f, .*/mkspecs/.*):\
                    !contains(f, .*/(qt_build_config|qt_parts|qt_configure|configure_base)\\.prf): \
                discard_from($$f)
        }
        # nobody's going to try to re-load the features above,
        # so don't bother with being selective.
        QMAKE_INTERNAL_INCLUDED_FEATURES = \
            # loading it gets simulated below.
            $$[QT_HOST_DATA/src]/mkspecs/features/device_config.prf \
            # must be delayed until qdevice.pri is ready.
            $$[QT_HOST_DATA/src]/mkspecs/features/mac/toolchain.prf \
            $$[QT_HOST_DATA/src]/mkspecs/features/toolchain.prf

        saved_variables = CONFIG QMAKE_CXXFLAGS
        for (name, saved_variables): \
            _SAVED_$$name = $$eval($$name)
        load(spec_pre)
        # qdevice.pri gets written too late (and we can't write it early
        # enough, as it's populated in stages, with later ones depending
        # on earlier ones). so inject its variables manually.
        for (l, $${currentConfig}.output.devicePro): \
            eval($$l)
        include($$QMAKESPEC/qmake.conf)
        load(spec_post)
        for (name, saved_variables): \
            $$name += $$eval(_SAVED_$$name)
        load(default_pre)

        # ensure pristine environment for configuration. again.
        discard_from($$[QT_HOST_DATA/get]/mkspecs/qconfig.pri)
        discard_from($$[QT_HOST_DATA/get]/mkspecs/qmodule.pri)
    }
}

defineTest(qtConfOutput_prepareSpec) {
    device = $$eval(config.input.device)
    !isEmpty(device) {
        devices = $$files($$[QT_HOST_DATA/src]/mkspecs/devices/*$$device*)
        isEmpty(devices): \
            qtConfFatalError("No device matching '$$device'.")
        !count(devices, 1) {
            err = "Multiple matches for device '$$device'. Candidates are:"
            for (d, devices): \
                err += "    $$basename(d)"
            qtConfFatalError($$err)
        }
        XSPEC = $$relative_path($$devices, $$[QT_HOST_DATA/src]/mkspecs)
    }
    xspec = $$eval(config.input.xplatform)
    !isEmpty(xspec) {
        !exists($$[QT_HOST_DATA/src]/mkspecs/$$xspec/qmake.conf): \
            qtConfFatalError("Invalid target platform '$$xspec'.")
        XSPEC = $$xspec
    }
    isEmpty(XSPEC): \
        XSPEC = $$[QMAKE_SPEC]
    export(XSPEC)
    QMAKESPEC = $$[QT_HOST_DATA/src]/mkspecs/$$XSPEC
    export(QMAKESPEC)

    notes = $$cat($$OUT_PWD/.config.notes, lines)
    !isEmpty(notes): \
        qtConfAddNote("Also available for $$notes")

    # deviceOptions() below contains conditionals coming form the spec,
    # so this cannot be delayed for a batch reload.
    reloadSpec()
}

defineTest(qtConfOutput_prepareOptions) {
    $${currentConfig}.output.devicePro += \
        $$replace(config.input.device-option, "^([^=]+) *= *(.*)$", "\\1 = \\2")
    darwin:!isEmpty(config.input.sdk) {
        $${currentConfig}.output.devicePro += \
            "QMAKE_MAC_SDK = $$val_escape(config.input.sdk)"
    }
    android {
        sdk_root = $$eval(config.input.android-sdk)
        isEmpty(sdk_root): \
            sdk_root = $$getenv(ANDROID_SDK_ROOT)
        isEmpty(sdk_root) {
            for(ever) {
                equals(QMAKE_HOST.os, Linux): \
                    sdk_root = $$(HOME)/Android/Sdk
                else: equals(QMAKE_HOST.os, Darwin): \
                    sdk_root = $$(HOME)/Library/Android/sdk
                else: \
                    break()
                !exists($$sdk_root): \
                    sdk_root =
                break()
            }
        }
        isEmpty(sdk_root): \
            qtConfFatalError("Cannot find Android SDK." \
                             "Please use -android-sdk option to specify one.")

        ndk_root = $$eval(config.input.android-ndk)
        isEmpty(ndk_root): \
            ndk_root = $$getenv(ANDROID_NDK_ROOT)
        isEmpty(ndk_root) {
            for(ever) {
                exists($$sdk_root/ndk-bundle) {
                    ndk_root = $$sdk_root/ndk-bundle
                    break()
                }
                equals(QMAKE_HOST.os, Linux): \
                    ndk_root = $$(HOME)/Android/Sdk/ndk-bundle
                else: equals(QMAKE_HOST.os, Darwin): \
                    ndk_root = $$(HOME)/Library/Android/sdk/ndk-bundle
                else: \
                    break()
                !exists($$ndk_root): \
                    ndk_root =
                break()
            }
        }
        isEmpty(ndk_root): \
            qtConfFatalError("Cannot find Android NDK." \
                             "Please use -android-ndk option to specify one.")

        ndk_tc_pfx = $$ndk_root/toolchains/llvm/prebuilt
        ndk_host = $$eval(config.input.android-ndk-host)
        isEmpty(ndk_host): \
            ndk_host = $$getenv(ANDROID_NDK_HOST)
        isEmpty(ndk_host) {
            equals(QMAKE_HOST.os, Linux) {
                ndk_host_64 = linux-x86_64
                ndk_host_32 = linux-x86
            } else: equals(QMAKE_HOST.os, Darwin) {
                ndk_host_64 = darwin-x86_64
                ndk_host_32 = darwin-x86
            } else: equals(QMAKE_HOST.os, Windows) {
                ndk_host_64 = windows-x86_64
                ndk_host_32 = windows
            } else {
                qtConfFatalError("Host operating system not supported by Android.")
            }
            !exists($$ndk_tc_pfx/$$ndk_host_64/*): ndk_host_64 =
            !exists($$ndk_tc_pfx/$$ndk_host_32/*): ndk_host_32 =
            equals(QMAKE_HOST.arch, x86_64):!isEmpty(ndk_host_64) {
                ndk_host = $$ndk_host_64
            } else: equals(QMAKE_HOST.arch, x86):!isEmpty(ndk_host_32) {
                ndk_host = $$ndk_host_32
            } else {
                !isEmpty(ndk_host_64): \
                    ndk_host = $$ndk_host_64
                else: !isEmpty(ndk_host_32): \
                    ndk_host = $$ndk_host_32
                else: \
                    qtConfFatalError("Cannot detect the Android host." \
                                     "Please use -android-ndk-host option to specify one.")
                qtConfAddNote("Available Android host does not match host architecture.")
            }
        } else {
            !exists($$ndk_tc_pfx/$$ndk_host/*) {
                err = "Specified Android NDK host '$$ndk_host' is invalid. Expected files in the following directory to exist:"
                err += '$${ndk_tc_pfx}/$${ndk_host}/'
                qtConfFatalError($$err)
            }
        }

        android_abis = $$eval(config.input.android-abis)
        isEmpty(android_abis): \
            android_abis = $$eval(config.input.android-arch)
        isEmpty(android_abis): \
            android_abis = armeabi-v7a,arm64-v8a,x86,x86_64
        platform = $$eval(config.input.android-ndk-platform)
        isEmpty(platform): \
            platform = android-21

        android_javac_target = $$eval(config.input.android-javac-target)
        android_javac_source = $$eval(config.input.android-javac-source)

        $${currentConfig}.output.devicePro += \
            "DEFAULT_ANDROID_SDK_ROOT = $$val_escape(sdk_root)" \
            "DEFAULT_ANDROID_NDK_ROOT = $$val_escape(ndk_root)" \
            "DEFAULT_ANDROID_PLATFORM = $$platform" \
            "DEFAULT_ANDROID_NDK_HOST = $$ndk_host" \
            "DEFAULT_ANDROID_ABIS = $$split(android_abis, ',')" \
            "ANDROID_JAVAC_TARGET_VERSION = $$android_javac_target" \
            "ANDROID_JAVAC_SOURCE_VERSION = $$android_javac_source"
    }

    export($${currentConfig}.output.devicePro)

    # if any settings were made, the spec will be reloaded later
    # to make them take effect.
}

defineTest(qtConfOutput_machineTuple) {
    $${currentConfig}.output.devicePro += \
        "GCC_MACHINE_DUMP = $$eval($${currentConfig}.tests.machineTuple.tuple)"
    export($${currentConfig}.output.devicePro)

    # for completeness, one could reload the spec here,
    # but no downstream users actually need that.
}

defineTest(qtConfOutput_commitOptions) {
    # qdevice.pri needs to be written early, because the compile tests require it.
    write_file($$QT_BUILD_TREE/mkspecs/qdevice.pri, $${currentConfig}.output.devicePro)|error()
}

# type (empty or 'host'), option name, default value
defineTest(processQtPath) {
    out_var = config.rel_input.$${2}
    path = $$eval(config.input.$${2})
    isEmpty(path) {
        $$out_var = $$3
    } else {
        path = $$absolute_path($$path, $$OUT_PWD)
        rel = $$relative_path($$path, $$eval(config.input.$${1}prefix))
        isEmpty(rel) {
            $$out_var = .
        } else: contains(rel, \.\..*) {
            !equals(2, sysconfdir) {
                PREFIX_COMPLAINTS += "-$$2 is not a subdirectory of -$${1}prefix."
                export(PREFIX_COMPLAINTS)
                !$$eval(have_$${1}prefix) {
                    PREFIX_REMINDER = true
                    export(PREFIX_REMINDER)
                }
            }
            $$out_var = $$path
        } else {
            $$out_var = $$rel
        }
    }
    export($$out_var)
}

defineTest(addConfStr) {
    QT_CONFIGURE_STR_OFFSETS += "    $$QT_CONFIGURE_STR_OFF,"
    QT_CONFIGURE_STRS += "    \"$$1\\0\""
    QT_CONFIGURE_STR_OFF = $$num_add($$QT_CONFIGURE_STR_OFF, $$str_size($$1), 1)
    export(QT_CONFIGURE_STR_OFFSETS)
    export(QT_CONFIGURE_STRS)
    export(QT_CONFIGURE_STR_OFF)
}

defineReplace(printInstallPath) {
    val = $$eval(config.rel_input.$$2)
    equals(val, $$3): return()
    return("$$1=$$val")
}

defineReplace(printInstallPaths) {
    ret = \
        $$printInstallPath(Documentation, docdir, doc) \
        $$printInstallPath(Headers, headerdir, include) \
        $$printInstallPath(Libraries, libdir, lib) \
        $$printInstallPath(LibraryExecutables, libexecdir, $$DEFAULT_LIBEXEC) \
        $$printInstallPath(Binaries, bindir, bin) \
        $$printInstallPath(Plugins, plugindir, plugins) \
        $$printInstallPath(Imports, importdir, imports) \
        $$printInstallPath(Qml2Imports, qmldir, qml) \
        $$printInstallPath(ArchData, archdatadir, .) \
        $$printInstallPath(Data, datadir, .) \
        $$printInstallPath(Translations, translationdir, translations) \
        $$printInstallPath(Examples, examplesdir, examples) \
        $$printInstallPath(Tests, testsdir, tests)
    return($$ret)
}

defineReplace(printHostPaths) {
    ret = \
        "HostPrefix=$$config.input.hostprefix" \
        $$printInstallPath(HostBinaries, hostbindir, bin) \
        $$printInstallPath(HostLibraries, hostlibdir, lib) \
        $$printInstallPath(HostData, hostdatadir, .) \
        "Sysroot=$$config.input.sysroot" \
        "SysrootifyPrefix=$$qmake_sysrootify" \
        "TargetSpec=$$XSPEC" \
        "HostSpec=$$[QMAKE_SPEC]"
    return($$ret)
}

defineTest(qtConfOutput_preparePaths) {
    isEmpty(config.input.prefix) {
        $$qtConfEvaluate("features.developer-build") {
            config.input.prefix = $$QT_BUILD_TREE  # In Development, we use sandboxed builds by default
        } else {
            win32: \
                config.input.prefix = C:/Qt/Qt-$$[QT_VERSION]
            else: \
                config.input.prefix = /usr/local/Qt-$$[QT_VERSION]
        }
        have_prefix = false
    } else {
        equals(XSPEC, $$[QMAKE_SPEC]) {
            # Only make the user-specified prefix absolute if we're not cross-compiling.
            config.input.prefix = $$absolute_path($$config.input.prefix, $$OUT_PWD)
        } else {
            # But we still must normalize path separators.
            config.input.prefix = $$replace(config.input.prefix, \\\\, /)
        }
        have_prefix = true
    }

    isEmpty(config.input.extprefix) {
        config.input.extprefix = $$config.input.prefix
        !isEmpty(config.input.sysroot): \
            qmake_sysrootify = true
        else: \
            qmake_sysrootify = false
    } else {
        config.input.extprefix = $$absolute_path($$config.input.extprefix, $$OUT_PWD)
        qmake_sysrootify = false
    }

    isEmpty(config.input.hostprefix) {
        $$qmake_sysrootify: \
            config.input.hostprefix = $$config.input.sysroot$$config.input.extprefix
        else: \
            config.input.hostprefix = $$config.input.extprefix
        have_hostprefix = false
    } else {
        isEqual(config.input.hostprefix, yes): \
            config.input.hostprefix = $$QT_BUILD_TREE
        else: \
            config.input.hostprefix = $$absolute_path($$config.input.hostprefix, $$OUT_PWD)
        have_hostprefix = true
    }

    equals(config.input.prefix, $$config.input.extprefix): \
        qmake_crossbuild = false
    else: \
        qmake_crossbuild = true

    PREFIX_COMPLAINTS =
    PREFIX_REMINDER = false
    win32: \
        DEFAULT_LIBEXEC = bin
    else: \
        DEFAULT_LIBEXEC = libexec
    darwin: \
        DEFAULT_SYSCONFDIR = /Library/Preferences/Qt
    else: \
        DEFAULT_SYSCONFDIR = etc/xdg

    processQtPath("", headerdir, include)
    processQtPath("", libdir, lib)
    processQtPath("", bindir, bin)
    processQtPath("", datadir, .)
    !equals(config.rel_input.datadir, .): \
        data_pfx = $$config.rel_input.datadir/
    processQtPath("", docdir, $${data_pfx}doc)
    processQtPath("", translationdir, $${data_pfx}translations)
    processQtPath("", examplesdir, $${data_pfx}examples)
    processQtPath("", testsdir, tests)
    processQtPath("", archdatadir, .)
    !equals(config.rel_input.archdatadir, .): \
        archdata_pfx = $$config.rel_input.archdatadir/
    processQtPath("", libexecdir, $${archdata_pfx}$$DEFAULT_LIBEXEC)
    processQtPath("", plugindir, $${archdata_pfx}plugins)
    processQtPath("", importdir, $${archdata_pfx}imports)
    processQtPath("", qmldir, $${archdata_pfx}qml)
    processQtPath("", sysconfdir, $$DEFAULT_SYSCONFDIR)
    $$have_hostprefix {
        processQtPath(host, hostbindir, bin)
        processQtPath(host, hostlibdir, lib)
        processQtPath(host, hostdatadir, .)
    } else {
        processQtPath(host, hostbindir, $$config.rel_input.bindir)
        processQtPath(host, hostlibdir, $$config.rel_input.libdir)
        processQtPath(host, hostdatadir, $$config.rel_input.archdatadir)
    }

    win32:$$qtConfEvaluate("features.shared") {
        # Windows DLLs are in the bin dir.
        libloc_absolute_path = $$absolute_path($$config.rel_input.bindir, $$config.input.prefix)
    } else {
        libloc_absolute_path = $$absolute_path($$config.rel_input.libdir, $$config.input.prefix)
    }
    config.input.liblocation_to_prefix = $$relative_path($$config.input.prefix, $$libloc_absolute_path)
    config.qtbase.features.shared.available =
    export(config.qtbase.features.shared.available)

    hostbindir_absolute_path = $$absolute_path($$config.rel_input.hostbindir, $$config.input.hostprefix)
    config.input.hostbindir_to_hostprefix = $$relative_path($$config.input.hostprefix, $$hostbindir_absolute_path)
    config.input.hostbindir_to_extprefix = $$relative_path($$config.input.extprefix, $$hostbindir_absolute_path)

    !isEmpty(PREFIX_COMPLAINTS) {
        PREFIX_COMPLAINTS = "$$join(PREFIX_COMPLAINTS, "$$escape_expand(\\n)Note: ")"
        $$PREFIX_REMINDER: \
            PREFIX_COMPLAINTS += "Maybe you forgot to specify -prefix/-hostprefix?"
        qtConfAddNote($$PREFIX_COMPLAINTS)
    }

    # populate qconfig.cpp (for qtcore)

    QT_CONFIGURE_STR_OFF = 0
    QT_CONFIGURE_STR_OFFSETS =
    QT_CONFIGURE_STRS =

    addConfStr($$config.rel_input.docdir)
    addConfStr($$config.rel_input.headerdir)
    addConfStr($$config.rel_input.libdir)
    addConfStr($$config.rel_input.libexecdir)
    addConfStr($$config.rel_input.bindir)
    addConfStr($$config.rel_input.plugindir)
    addConfStr($$config.rel_input.importdir)
    addConfStr($$config.rel_input.qmldir)
    addConfStr($$config.rel_input.archdatadir)
    addConfStr($$config.rel_input.datadir)
    addConfStr($$config.rel_input.translationdir)
    addConfStr($$config.rel_input.examplesdir)
    addConfStr($$config.rel_input.testsdir)

    QT_CONFIGURE_STR_OFFSETS_ALL = $$QT_CONFIGURE_STR_OFFSETS
    QT_CONFIGURE_STRS_ALL = $$QT_CONFIGURE_STRS
    QT_CONFIGURE_STR_OFFSETS =
    QT_CONFIGURE_STRS =

    addConfStr($$config.input.sysroot)
    addConfStr($$qmake_sysrootify)
    addConfStr($$config.rel_input.hostbindir)
    addConfStr($$config.rel_input.hostlibdir)
    addConfStr($$config.rel_input.hostdatadir)
    addConfStr($$XSPEC)
    addConfStr($$[QMAKE_SPEC])

    $${currentConfig}.output.qconfigSource = \
        "/* Installation Info */" \
        "static const char qt_configure_prefix_path_str  [12+256] = \"qt_prfxpath=$$config.input.prefix\";" \
        "$${LITERAL_HASH}ifdef QT_BUILD_QMAKE" \
        "static const char qt_configure_ext_prefix_path_str   [12+256] = \"qt_epfxpath=$$config.input.extprefix\";" \
        "static const char qt_configure_host_prefix_path_str  [12+256] = \"qt_hpfxpath=$$config.input.hostprefix\";" \
        "$${LITERAL_HASH}endif" \
        "" \
        "static const short qt_configure_str_offsets[] = {" \
        $$QT_CONFIGURE_STR_OFFSETS_ALL \
        "$${LITERAL_HASH}ifdef QT_BUILD_QMAKE" \
        $$QT_CONFIGURE_STR_OFFSETS \
        "$${LITERAL_HASH}endif" \
        "};" \
        "static const char qt_configure_strs[] =" \
        $$QT_CONFIGURE_STRS_ALL \
        "$${LITERAL_HASH}ifdef QT_BUILD_QMAKE" \
        $$QT_CONFIGURE_STRS \
        "$${LITERAL_HASH}endif" \
        ";" \
        "" \
        "$${LITERAL_HASH}define QT_CONFIGURE_SETTINGS_PATH \"$$config.rel_input.sysconfdir\"" \
        "$${LITERAL_HASH}define QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH \"$$config.input.liblocation_to_prefix\"" \
        "$${LITERAL_HASH}define QT_CONFIGURE_HOSTBINDIR_TO_EXTPREFIX_PATH \"$$config.input.hostbindir_to_extprefix\"" \
        "$${LITERAL_HASH}define QT_CONFIGURE_HOSTBINDIR_TO_HOSTPREFIX_PATH \"$$config.input.hostbindir_to_hostprefix\"" \
        "" \
        "$${LITERAL_HASH}ifdef QT_BUILD_QMAKE" \
        "$${LITERAL_HASH} define QT_CONFIGURE_SYSROOTIFY_PREFIX $$qmake_sysrootify" \
        "$${LITERAL_HASH} define QT_CONFIGURE_CROSSBUILD $$qmake_crossbuild" \
        "$${LITERAL_HASH}endif" \
        "" \
        "$${LITERAL_HASH}define QT_CONFIGURE_PREFIX_PATH qt_configure_prefix_path_str + 12" \
        "$${LITERAL_HASH}ifdef QT_BUILD_QMAKE" \
        "$${LITERAL_HASH} define QT_CONFIGURE_EXT_PREFIX_PATH qt_configure_ext_prefix_path_str + 12" \
        "$${LITERAL_HASH} define QT_CONFIGURE_HOST_PREFIX_PATH qt_configure_host_prefix_path_str + 12" \
        "$${LITERAL_HASH}endif"
    export($${currentConfig}.output.qconfigSource)

    # create bin/qt.conf. this doesn't use the regular file output
    # mechanism, as the file is relied upon by configure tests.

    cont = \
        "[EffectivePaths]" \
        "Prefix=.." \
        "[DevicePaths]" \
        "Prefix=$$config.input.prefix" \
        $$printInstallPaths() \
        "[Paths]" \
        "Prefix=$$config.input.extprefix" \
        $$printInstallPaths() \
        $$printHostPaths()
    !equals(QT_SOURCE_TREE, $$QT_BUILD_TREE): \
        cont += \
            "[EffectiveSourcePaths]" \
            "Prefix=$$[QT_INSTALL_PREFIX/src]"
    write_file($$QT_BUILD_TREE/bin/qt.conf, cont)|error()
    reload_properties()

    # if a sysroot was configured, the spec will be reloaded later,
    # as some specs contain $$[SYSROOT] references.
}

defineTest(qtConfOutput_reloadSpec) {
    !isEmpty($${currentConfig}.output.devicePro)| \
            !isEmpty(config.input.sysroot): \
        reloadSpec()

    # toolchain.prf uses this.
    dummy = $$qtConfEvaluate("features.cross_compile")

    bypassNesting() {
        QMAKE_INTERNAL_INCLUDED_FEATURES -= \
            $$[QT_HOST_DATA/src]/mkspecs/features/mac/toolchain.prf \
            $$[QT_HOST_DATA/src]/mkspecs/features/toolchain.prf
        load(toolchain)
    }
}

defineTest(qtConfOutput_shared) {
    !$${2}: return()

    # export this here, so later tests can use it
    CONFIG += shared
    export(CONFIG)
}

defineTest(qtConfOutput_sanitizer) {
    !$${2}: return()

    # Export this here, so that WebEngine can access it at configure time.
    CONFIG += sanitizer
    $$qtConfEvaluate("features.sanitize_address"): CONFIG += sanitize_address
    $$qtConfEvaluate("features.sanitize_thread"): CONFIG += sanitize_thread
    $$qtConfEvaluate("features.sanitize_memory"): CONFIG += sanitize_memory
    $$qtConfEvaluate("features.sanitize_undefined"): CONFIG += sanitize_undefined

    export(CONFIG)
}

defineTest(qtConfOutput_architecture) {
    arch = $$qtConfEvaluate("tests.architecture.arch")
    subarch = $$qtConfEvaluate('tests.architecture.subarch')
    buildabi = $$qtConfEvaluate("tests.architecture.buildabi")

    $$qtConfEvaluate("features.cross_compile") {
        host_arch = $$qtConfEvaluate("tests.host_architecture.arch")
        host_buildabi = $$qtConfEvaluate("tests.host_architecture.buildabi")

        privatePro = \
            "host_build {" \
            "    QT_CPU_FEATURES.$$host_arch = $$qtConfEvaluate('tests.host_architecture.subarch')" \
            "} else {" \
            "    QT_CPU_FEATURES.$$arch = $$subarch" \
            "}"
        publicPro = \
            "host_build {" \
            "    QT_ARCH = $$host_arch" \
            "    QT_BUILDABI = $$host_buildabi" \
            "    QT_TARGET_ARCH = $$arch" \
            "    QT_TARGET_BUILDABI = $$buildabi" \
            "} else {" \
            "    QT_ARCH = $$arch" \
            "    QT_BUILDABI = $$buildabi" \
            "}"

    } else {
        privatePro = \
            "QT_CPU_FEATURES.$$arch = $$subarch"
        publicPro = \
            "QT_ARCH = $$arch" \
            "QT_BUILDABI = $$buildabi"
    }

    $${currentConfig}.output.publicPro += $$publicPro
    export($${currentConfig}.output.publicPro)
    $${currentConfig}.output.privatePro += $$privatePro
    export($${currentConfig}.output.privatePro)

    # setup QT_ARCH and QT_CPU_FEATURES variables used by qtConfEvaluate
    QT_ARCH = $$arch
    export(QT_ARCH)
    QT_CPU_FEATURES.$$arch = $$subarch
    export(QT_CPU_FEATURES.$$arch)
}

defineTest(qtConfOutput_qreal) {
    qreal = $$config.input.qreal
    isEmpty(qreal): qreal = "double"
    qreal_string = $$replace(qreal, [^a-zA-Z0-9], "_")
    qtConfOutputVar(assign, "privatePro", "QT_COORD_TYPE", $$qreal)
    !equals(qreal, "double") {
        qtConfOutputSetDefine("publicHeader", "QT_COORD_TYPE", $$qreal)
        qtConfOutputSetDefine("publicHeader", "QT_COORD_TYPE_STRING", "\"$$qreal_string\"")
    }
}

defineTest(qtConfOutput_pkgConfig) {
    !$${2}: return()

    PKG_CONFIG_EXECUTABLE = $$eval($${currentConfig}.tests.pkg-config.pkgConfig)
    qtConfOutputVar(assign, "privatePro", "PKG_CONFIG_EXECUTABLE", $$PKG_CONFIG_EXECUTABLE)
    export(PKG_CONFIG_EXECUTABLE)
    # this method also exports PKG_CONFIG_(LIB|SYSROOT)DIR, so that tests using pkgConfig will work correctly
    PKG_CONFIG_SYSROOT_DIR = $$eval($${currentConfig}.tests.pkg-config.pkgConfigSysrootDir)
    !isEmpty(PKG_CONFIG_SYSROOT_DIR) {
        qtConfOutputVar(assign, "publicPro", "PKG_CONFIG_SYSROOT_DIR", $$PKG_CONFIG_SYSROOT_DIR)
        export(PKG_CONFIG_SYSROOT_DIR)
    }
    PKG_CONFIG_LIBDIR = $$eval($${currentConfig}.tests.pkg-config.pkgConfigLibdir)
    !isEmpty(PKG_CONFIG_LIBDIR) {
        qtConfOutputVar(assign, "publicPro", "PKG_CONFIG_LIBDIR", $$PKG_CONFIG_LIBDIR)
        export(PKG_CONFIG_LIBDIR)
    }
}

defineTest(qtConfOutput_crossCompile) {
    !$${2}: return()

    # We need to preempt the output here, as subsequent tests rely on it
    CONFIG += cross_compile
    export(CONFIG)
}

defineTest(qtConfOutput_useBFDLinker) {
    !$${2}: return()

    # We need to preempt the output here, so that qtConfTest_linkerSupportsFlag can work properly in qtbase
    CONFIG += use_bfd_linker
    export(CONFIG)
}

defineTest(qtConfOutput_useGoldLinker) {
    !$${2}: return()

    # We need to preempt the output here, so that qtConfTest_linkerSupportsFlag can work properly in qtbase
    CONFIG += use_gold_linker
    export(CONFIG)
}

defineTest(qtConfOutput_useLLDLinker) {
    !$${2}: return()

    # We need to preempt the output here, so that qtConfTest_linkerSupportsFlag can work properly in qtbase
    CONFIG += use_lld_linker
    export(CONFIG)
}

defineTest(qtConfOutput_debugAndRelease) {
    $$qtConfEvaluate("features.debug") {
        qtConfOutputVar(append, "publicPro", "CONFIG", "debug")
        $${2}: qtConfOutputVar(append, "publicPro", "QT_CONFIG", "release")
        qtConfOutputVar(append, "publicPro", "QT_CONFIG", "debug")
    } else {
        qtConfOutputVar(append, "publicPro", "CONFIG", "release")
        $${2}: qtConfOutputVar(append, "publicPro", "QT_CONFIG", "debug")
        qtConfOutputVar(append, "publicPro", "QT_CONFIG", "release")
    }
}

defineTest(qtConfOutput_compilerFlags) {
    # this output also exports the variables locally, so that subsequent compiler tests can use them

    output =
    !isEmpty(config.input.wflags) {
        wflags = $$join(config.input.wflags, " -W", "-W")
        QMAKE_CFLAGS_WARN_ON += $$wflags
        QMAKE_CXXFLAGS_WARN_ON += $$wflags
        export(QMAKE_CFLAGS_WARN_ON)
        export(QMAKE_CXXFLAGS_WARN_ON)
        output += \
            "QMAKE_CFLAGS_WARN_ON += $$wflags" \
            "QMAKE_CXXFLAGS_WARN_ON += $$wflags"
    }
    !isEmpty(config.input.defines) {
        EXTRA_DEFINES += $$config.input.defines
        export(EXTRA_DEFINES)
        output += "EXTRA_DEFINES += $$val_escape(config.input.defines)"
    }
    !isEmpty(config.input.includes) {
        EXTRA_INCLUDEPATH += $$config.input.includes
        export(EXTRA_INCLUDEPATH)
        output += "EXTRA_INCLUDEPATH += $$val_escape(config.input.includes)"
    }

    !isEmpty(config.input.lpaths) {
        EXTRA_LIBDIR += $$config.input.lpaths
        export(EXTRA_LIBDIR)
        output += "EXTRA_LIBDIR += $$val_escape(config.input.lpaths)"
    }
    darwin:!isEmpty(config.input.fpaths) {
        EXTRA_FRAMEWORKPATH += $$config.input.fpaths
        export(EXTRA_FRAMEWORKPATH)
        output += "EXTRA_FRAMEWORKPATH += $$val_escape(config.input.fpaths)"
    }

    $${currentConfig}.output.privatePro += $$output
    export($${currentConfig}.output.privatePro)
}

defineTest(qtConfOutput_gccSysroot) {
    !$${2}: return()

    # This variable also needs to be exported immediately, so the compilation tests
    # can pick it up.
    EXTRA_QMAKE_ARGS += \
        "\"QMAKE_CFLAGS += --sysroot=$$config.input.sysroot\"" \
        "\"QMAKE_CXXFLAGS += --sysroot=$$config.input.sysroot\"" \
        "\"QMAKE_LFLAGS += --sysroot=$$config.input.sysroot\""
    export(EXTRA_QMAKE_ARGS)

    # This one is for qtConfToolchainSupportsFlag().
    QMAKE_CXXFLAGS += --sysroot=$$config.input.sysroot
    export(QMAKE_CXXFLAGS)

    output = \
        "!host_build {" \
        "    QMAKE_CFLAGS    += --sysroot=\$\$[QT_SYSROOT]" \
        "    QMAKE_CXXFLAGS  += --sysroot=\$\$[QT_SYSROOT]" \
        "    QMAKE_LFLAGS    += --sysroot=\$\$[QT_SYSROOT]" \
        "}"
    $${currentConfig}.output.publicPro += $$output
    export($${currentConfig}.output.publicPro)
}

defineTest(qtConfOutput_qmakeArgs) {
    !$${2}: return()

    $${currentConfig}.output.privatePro += "!host_build|!cross_compile {"
    for (a, config.input.qmakeArgs) {
        $${currentConfig}.output.privatePro += "    $$a"
        EXTRA_QMAKE_ARGS += $$system_quote($$a)
    }
    $${currentConfig}.output.privatePro += "}"
    export(EXTRA_QMAKE_ARGS)
    export($${currentConfig}.output.privatePro)
}

defineReplace(qtConfOutputPostProcess_publicPro) {
    qt_version = $$[QT_VERSION]
    output = \
        $$1 \
        "QT_VERSION = $$qt_version" \
        "QT_MAJOR_VERSION = $$section(qt_version, '.', 0, 0)" \
        "QT_MINOR_VERSION = $$section(qt_version, '.', 1, 1)" \
        "QT_PATCH_VERSION = $$section(qt_version, '.', 2, 2)"

    #libinfix and namespace
    !isEmpty(config.input.qt_libinfix): output += "QT_LIBINFIX = $$config.input.qt_libinfix"
    !isEmpty(config.input.qt_namespace): output += "QT_NAMESPACE = $$config.input.qt_namespace"

    !isEmpty(QMAKE_GCC_MAJOR_VERSION) {
        output += \
            "QT_GCC_MAJOR_VERSION = $$QMAKE_GCC_MAJOR_VERSION" \
            "QT_GCC_MINOR_VERSION = $$QMAKE_GCC_MINOR_VERSION" \
            "QT_GCC_PATCH_VERSION = $$QMAKE_GCC_PATCH_VERSION"
    }
    !isEmpty(QMAKE_MAC_SDK_VERSION): \
        output += "QT_MAC_SDK_VERSION = $$QMAKE_MAC_SDK_VERSION"
    !isEmpty(QMAKE_CLANG_MAJOR_VERSION) {
        output += \
            "QT_CLANG_MAJOR_VERSION = $$QMAKE_CLANG_MAJOR_VERSION" \
            "QT_CLANG_MINOR_VERSION = $$QMAKE_CLANG_MINOR_VERSION" \
            "QT_CLANG_PATCH_VERSION = $$QMAKE_CLANG_PATCH_VERSION"
    }
    !isEmpty(QMAKE_APPLE_CLANG_MAJOR_VERSION) {
        output += \
            "QT_APPLE_CLANG_MAJOR_VERSION = $$QMAKE_APPLE_CLANG_MAJOR_VERSION" \
            "QT_APPLE_CLANG_MINOR_VERSION = $$QMAKE_APPLE_CLANG_MINOR_VERSION" \
            "QT_APPLE_CLANG_PATCH_VERSION = $$QMAKE_APPLE_CLANG_PATCH_VERSION"
    }
    !isEmpty(QMAKE_MSC_VER) {
        output += \
            "QT_MSVC_MAJOR_VERSION = $$replace(QMAKE_MSC_FULL_VER, "(..)(..)(.*)", "\\1")" \
            "QT_MSVC_MINOR_VERSION = $$format_number($$replace(QMAKE_MSC_FULL_VER, "(..)(..)(.*)", "\\2"))" \
            "QT_MSVC_PATCH_VERSION = $$replace(QMAKE_MSC_FULL_VER, "(..)(..)(.*)", "\\3")"
    }
    !isEmpty(QMAKE_ICC_VER) {
        output += \
            "QT_ICC_MAJOR_VERSION = $$replace(QMAKE_ICC_VER, "(..)(..)", "\\1")" \
            "QT_ICC_MINOR_VERSION = $$format_number($$replace(QMAKE_ICC_VER, "(..)(..)", "\\2"))" \
            "QT_ICC_PATCH_VERSION = $$QMAKE_ICC_UPDATE_VER"
    }
    !isEmpty(QMAKE_GHS_VERSION) {
        output += \
            "QT_GHS_MAJOR_VERSION = $$replace(QMAKE_GHS_VERSION, "(.*)(.)(.)", "\\1")" \
            "QT_GHS_MINOR_VERSION = $$replace(QMAKE_GHS_VERSION, "(.*)(.)(.)", "\\2")" \
            "QT_GHS_PATCH_VERSION = $$replace(QMAKE_GHS_VERSION, "(.*)(.)(.)", "\\3")"
    }

    output += "QT_EDITION = $$config.input.qt_edition"
    !contains(config.input.qt_edition, "(OpenSource|Preview)") {
        output += \
            "QT_LICHECK = $$config.input.qt_licheck" \
            "QT_RELEASE_DATE = $$config.input.qt_release_date"
    }

    wasm: {
        qt_emcc_version = $$qtSystemEmccVersion()
        output += \
           "QT_EMCC_VERSION = $$qt_emcc_version"
    }

    return($$output)
}

defineReplace(qtConfOutputPostProcess_privatePro) {
    output = $$1

    !isEmpty(config.input.external-hostbindir): \
        output += "HOST_QT_TOOLS = $$val_escape(config.input.external-hostbindir)"

    return($$output)
}

defineReplace(qtConfOutputPostProcess_publicHeader) {
    qt_version = $$[QT_VERSION]
    output = \
        $$1 \
        "$${LITERAL_HASH}define QT_VERSION_STR \"$$qt_version\"" \
        "$${LITERAL_HASH}define QT_VERSION_MAJOR $$section(qt_version, '.', 0, 0)" \
        "$${LITERAL_HASH}define QT_VERSION_MINOR $$section(qt_version, '.', 1, 1)" \
        "$${LITERAL_HASH}define QT_VERSION_PATCH $$section(qt_version, '.', 2, 2)"

    !$$qtConfEvaluate("features.shared") {
        output += \
            "/* Qt was configured for a static build */" \
            "$${LITERAL_HASH}if !defined(QT_SHARED) && !defined(QT_STATIC)" \
            "$${LITERAL_HASH} define QT_STATIC" \
            "$${LITERAL_HASH}endif"
    }

    !isEmpty(config.input.qt_libinfix): \
        output += "$${LITERAL_HASH}define QT_LIBINFIX \"$$eval(config.input.qt_libinfix)\""

    wasm: {
        qt_emcc_version = $$qtSystemEmccVersion()
output += \
           "$${LITERAL_HASH}define QT_EMCC_VERSION \"$$qt_emcc_version\""
    }

    return($$output)
}


# custom reporting

defineTest(qtConfReport_buildParts) {
    qtConfReportPadded($${1}, $$qtConfEvaluate("tests.build_parts.value"))
}

defineReplace(qtConfReportArch) {
    arch = $$qtConfEvaluate('tests.$${1}.arch')
    subarch = $$qtConfEvaluate('tests.$${1}.subarch')
    isEmpty(subarch): subarch = <none>
    return("$$arch, CPU features: $$subarch")
}

defineReplace(qtConfReportCompiler) {
    clang_cl: {
        return("clang-cl $${QMAKE_CLANG_MAJOR_VERSION}.$${QMAKE_CLANG_MINOR_VERSION}.$${QMAKE_CLANG_PATCH_VERSION}")
    } else: clang {
        !isEmpty(QMAKE_APPLE_CLANG_MAJOR_VERSION) {
            return("clang (Apple) $${QMAKE_APPLE_CLANG_MAJOR_VERSION}.$${QMAKE_APPLE_CLANG_MINOR_VERSION}.$${QMAKE_APPLE_CLANG_PATCH_VERSION}")
        } else {
            return("clang $${QMAKE_CLANG_MAJOR_VERSION}.$${QMAKE_CLANG_MINOR_VERSION}.$${QMAKE_CLANG_PATCH_VERSION}")
        }
    } else: intel_icc {
        return("intel_icc $$QMAKE_ICC_VER")
    } else: intel_icl {
        return("intel_icl $$QMAKE_ICC_VER")
    } else: rim_qcc {
        return("rim_qcc $${QMAKE_GCC_MAJOR_VERSION}.$${QMAKE_GCC_MINOR_VERSION}.$${QMAKE_GCC_PATCH_VERSION}")
    } else: gcc {
        return("gcc $${QMAKE_GCC_MAJOR_VERSION}.$${QMAKE_GCC_MINOR_VERSION}.$${QMAKE_GCC_PATCH_VERSION}")
    } else: msvc {
        return("msvc $$QMAKE_MSC_FULL_VER")
    } else: ghs {
        return("ghs $$QMAKE_GHS_VERSION")
    } else {
        return("unknown ($$QMAKE_COMPILER)")
    }
}


defineTest(qtConfReport_buildTypeAndConfig) {
    !$$qtConfEvaluate("features.cross_compile") {
        qtConfAddReport("Build type: $$[QMAKE_SPEC] ($$qtConfReportArch(architecture))")
        qtConfAddReport("Compiler: $$qtConfReportCompiler()")
    } else {
        qtConfAddReport("Building on: $$[QMAKE_SPEC] ($$qtConfReportArch(host_architecture))")
        qtConfAddReport("Building for: $$[QMAKE_XSPEC] ($$qtConfReportArch(architecture))")
        qtConfAddReport("Target compiler: $$qtConfReportCompiler()")
    }

    qtConfAddReport()
    qtConfAddReport("Configuration: $$eval($${currentConfig}.output.privatePro.append.CONFIG) $$eval($${currentConfig}.output.publicPro.append.QT_CONFIG)")
    qtConfAddReport()
}

defineTest(qtConfReport_buildMode) {
    $$qtConfEvaluate("features.force_debug_info"): \
        release = "release (with debug info)"
    else: \
        release = "release"

    $$qtConfEvaluate("features.debug") {
        build_mode = "debug"
        raw_build_mode = "debug"
    } else {
        build_mode = $$release
        raw_build_mode = "release"
    }

    $$qtConfEvaluate("features.debug_and_release"): \
        build_mode = "debug and $$release; default link: $$raw_build_mode"

    $$qtConfEvaluate("features.release_tools"): \
        build_mode = "$$build_mode; optimized tools"

    qtConfReportPadded($$1, $$build_mode)
}

defineTest(qtConfReport_emccVersion) {
    EMCC_VERSION = $$qtSystemEmccVersion()
    REQ_VERSION = $$qtEmccRecommendedVersion()
    !equals(EMCC_VERSION, $$REQ_VERSION) {
        qtConfAddReport("You should use the recommended Emscripten version $$REQ_VERSION with this Qt. You have $$EMCC_VERSION $$QT_EMCC_VERSION")
    }
}

# ensure pristine environment for configuration
discard_from($$[QT_HOST_DATA/get]/mkspecs/qconfig.pri)
discard_from($$[QT_HOST_DATA/get]/mkspecs/qmodule.pri)
# ... and cause them to be reloaded afterwards
QMAKE_POST_CONFIGURE += \
    "include(\$\$[QT_HOST_DATA/get]/mkspecs/qconfig.pri)" \
    "include(\$\$[QT_HOST_DATA/get]/mkspecs/qmodule.pri)"

defineTest(createConfigStatus) {
    $$QMAKE_REDO_CONFIG: return()
    cfg = $$relative_path($$_PRO_FILE_PWD_/configure, $$OUT_PWD)
    ext =
    equals(QMAKE_HOST.os, Windows) {
        ext = .bat
        cont = \
            "$$system_quote($$system_path($$cfg)$$ext) -redo %*"
    } else {
        !contains(cfg, .*/.*): cfg = ./$$cfg
        cont = \
            "$${LITERAL_HASH}!/bin/sh" \
            "exec $$system_quote($$cfg) -redo \"$@\""
    }
    write_file($$OUT_PWD/config.status$$ext, cont, exe)|error()
}

QMAKE_POST_CONFIGURE += \
    "createConfigStatus()"
