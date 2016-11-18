# custom command line handling

defineTest(qtConfCommandline_qmakeArgs) {
    contains(1, QMAKE_[A-Z_]+ *[-+]?=.*) {
        config.input.qmakeArgs += $$1
        export(config.input.qmakeArgs)
        return(true)
    }
    return(false)
}

defineTest(qtConfCommandline_cxxstd) {
    msvc: \
        qtConfAddError("Command line option -c++std is not supported with MSVC compilers.")

    arg = $${1}
    val = $${2}
    isEmpty(val): val = $$qtConfGetNextCommandlineArg()
    !contains(val, "^-.*"):!isEmpty(val) {
        contains(val, "(c\+\+)?11") {
            qtConfCommandlineSetInput("c++14", "no")
        } else: contains(val, "(c\+\+)?(14|1y)") {
            qtConfCommandlineSetInput("c++14", "yes")
            qtConfCommandlineSetInput("c++1z", "no")
        } else: contains(val, "(c\+\+)?(1z)") {
            qtConfCommandlineSetInput("c++14", "yes")
            qtConfCommandlineSetInput("c++1z", "yes")
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
        } else: equals(val, "undefined") {
            qtConfCommandlineSetInput("sanitize_undefined", "yes")
        } else {
            qtConfAddError("Invalid argument $$val to command line parameter $$arg")
        }
    } else {
        qtConfAddError("Missing argument to command line parameter $$arg")
    }
}

# callbacks

defineReplace(qtConfFunc_crossCompile) {
    !isEmpty(config.input.sysroot): return(true)
    spec = $$[QMAKE_SPEC]
    !equals(spec, $$[QMAKE_XSPEC]): return(true)
    return(false)
}

# custom tests

defineTest(qtConfTest_architecture) {
    !qtConfTest_compile($${1}): \
        error("Could not determine $$eval($${1}.label). See config.log for details.")

    test = $$eval($${1}.test)
    test_out_dir = $$shadowed($$QMAKE_CONFIG_TESTS_DIR/$$test)
    unix:exists($$test_out_dir/arch): \
        content = $$cat($$test_out_dir/arch, blob)
    else: win32:exists($$test_out_dir/arch.exe): \
        content = $$cat($$test_out_dir/arch.exe, blob)
    else: android:exists($$test_out_dir/libarch.so): \
        content = $$cat($$test_out_dir/libarch.so, blob)
    else: \
        error("$$eval($${1}.label) detection binary not found.")

    arch_magic = ".*==Qt=magic=Qt== Architecture:([^\\0]*).*"
    subarch_magic = ".*==Qt=magic=Qt== Sub-architecture:([^\\0]*).*"

    !contains(content, $$arch_magic)|!contains(content, $$subarch_magic): \
        error("$$eval($${1}.label) detection binary does not contain expected data.")

    $${1}.arch = $$replace(content, $$arch_magic, "\\1")
    $${1}.subarch = $$replace(content, $$subarch_magic, "\\1")
    $${1}.subarch = $$split($${1}.subarch, " ")
    export($${1}.arch)
    export($${1}.subarch)
    qtLog("Detected architecture: $$eval($${1}.arch) ($$eval($${1}.subarch))")

    $${1}.cache += arch subarch
    export($${1}.cache)
    return(true)
}

defineTest(qtConfTest_avx_test_apple_clang) {
    !*g++*:!*-clang*: return(true)

    qtRunLoggedCommand("$$QMAKE_CXX --version", compiler)|return(false)
    contains(compiler, "Apple clang version [23]") {
        # Some clang versions produce internal compiler errors compiling Qt AVX code
        return(false)
    } else {
        return(true)
    }
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
            gcc {
                qtRunLoggedCommand("$$QMAKE_CXX -dumpmachine", gccMachineDump): \
                        !isEmpty(gccMachineDump): \
                    pkgConfigLibdir = "$$pkgConfigLibdir:$$sysroot/usr/lib/$$gccMachineDump/pkgconfig"
            }

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

defineTest(qtConfTest_neon) {
    contains($${currentConfig}.tests.architecture.subarch, "neon"): return(true)
    return(false)
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

defineTest(qtConfTest_checkCompiler) {
    contains(QMAKE_CXX, ".*clang.*") {
        qtRunLoggedCommand("$$QMAKE_CXX -v 2>&1", versionstr)|return(false)
        versionstr = "$$versionstr"
        contains(versionstr, "^Apple (clang|LLVM) version .*") {
            $${1}.compilerDescription = "Apple Clang"
            $${1}.compilerId = "apple_clang"
            $${1}.compilerVersion = $$replace(versionstr, "^Apple (clang|LLVM) version ([0-9.]+).*$", "\\2")
        } else: contains(versionstr, ".*clang version.*") {
            $${1}.compilerDescription = "Clang"
            $${1}.compilerId = "clang"
            $${1}.compilerVersion = $$replace(versionstr, "^.*clang version ([0-9.]+).*", "\\1")
        } else {
            return(false)
        }
    } else: contains(QMAKE_CXX, ".*g\\+\\+.*") {
        qtRunLoggedCommand("$$QMAKE_CXX -dumpversion", version)|return(false)
        $${1}.compilerDescription = "GCC"
        $${1}.compilerId = "gcc"
        $${1}.compilerVersion = $$version
    } else: contains(QMAKE_CXX, ".*icpc") {
        qtRunLoggedCommand("$$QMAKE_CXX -dumpversion", version)|return(false)
        $${1}.compilerDescription = "ICC"
        $${1}.compilerId = "icc"
        $${1}.compilerVersion = $$version
    } else: msvc {
        command = $$QMAKE_CXX /EP /nologo $$source $$system_quote($$QMAKE_CONFIG_TESTS_DIR/win/msvc_version.cpp)
        qtRunLoggedCommand("$$command", version)|return(false)
        version = "$$version"
        $${1}.compilerDescription = "MSVC"
        $${1}.compilerId = "cl"
        $${1}.compilerVersion = $$replace(version, "^.*([0-9]{2})([0-9]{2})([0-9]{5}).*$", "\\1.\\2.\\3")
    } else {
        return(false)
    }
    $${1}.compilerDescription += $$eval($${1}.compilerVersion)
    export($${1}.compilerDescription)
    export($${1}.compilerId)
    export($${1}.compilerVersion)
    $${1}.cache += compilerDescription compilerId compilerVersion
    export($${1}.cache)
    return(true)
}

# custom outputs

defineTest(qtConfOutput_shared) {
    !$${2}: return()

    # export this here, so later tests can use it
    CONFIG += shared
    export(CONFIG)
}

defineTest(qtConfOutput_architecture) {
    arch = $$qtConfEvaluate("tests.architecture.arch")

    $$qtConfEvaluate("features.cross_compile") {
        host_arch = $$qtConfEvaluate("tests.host_architecture.arch")

        privatePro = \
            "host_build {" \
            "    QT_CPU_FEATURES.$$host_arch = $$qtConfEvaluate('tests.host_architecture.subarch')" \
            "} else {" \
            "    QT_CPU_FEATURES.$$arch = $$qtConfEvaluate('tests.architecture.subarch')" \
            "}"
        publicPro = \
            "host_build {" \
            "    QT_ARCH = $$host_arch" \
            "    QT_TARGET_ARCH = $$arch" \
            "} else {" \
            "    QT_ARCH = $$arch" \
            "}"

    } else {
        privatePro = \
            "QT_CPU_FEATURES.$$arch = $$qtConfEvaluate('tests.architecture.subarch')"
        publicPro = \
            "QT_ARCH = $$arch"
    }

    $${currentConfig}.output.publicPro += $$publicPro
    export($${currentConfig}.output.publicPro)
    $${currentConfig}.output.privatePro += $$privatePro
    export($${currentConfig}.output.privatePro)

    # setup QT_ARCH variable used by qtConfEvaluate
    QT_ARCH = $$arch
    export(QT_ARCH)
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

defineTest(qtConfOutput_useGoldLinker) {
    !$${2}: return()

    # We need to preempt the output here, so that qtConfTest_linkerSupportsFlag can work properly in qtbase
    CONFIG += use_gold_linker
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

defineTest(qtConfOutput_compilerVersion) {
    !$${2}: return()

    name = $$upper($$eval($${currentConfig}.tests.compiler.compilerId))
    version = $$eval($${currentConfig}.tests.compiler.compilerVersion)
    major = $$section(version, '.', 0, 0)
    minor = $$section(version, '.', 1, 1)
    patch = $$section(version, '.', 2, 2)
    isEmpty(minor): minor = 0
    isEmpty(patch): patch = 0

    $${currentConfig}.output.publicPro += \
        "QT_$${name}_MAJOR_VERSION = $$major" \
        "QT_$${name}_MINOR_VERSION = $$minor" \
        "QT_$${name}_PATCH_VERSION = $$patch"

    export($${currentConfig}.output.publicPro)
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

    $${currentConfig}.output.privatePro = "!host_build {"
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

    output += "QT_EDITION = $$config.input.qt_edition"
    !contains(config.input.qt_edition, "(OpenSource|Preview)") {
        output += \
            "QT_LICHECK = $$config.input.qt_licheck" \
            "QT_RELEASE_DATE = $$config.input.qt_release_date"
    }

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

    return($$output)
}


# custom reporting

defineTest(qtConfReport_buildParts) {
    qtConfReportPadded($${1}, $$qtConfEvaluate("tests.build_parts.value"))
}

defineTest(qtConfReport_buildTypeAndConfig) {
    !$$qtConfEvaluate("features.cross_compile") {
        qtConfAddReport("Build type: $$qtConfEvaluate('tests.architecture.arch')")
    } else {
        qtConfAddReport("Building on:  $$qtConfEvaluate('tests.host_architecture.arch')")
        qtConfAddReport("Building for: $$qtConfEvaluate('tests.architecture.arch')")
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

# ensure pristine environment for configuration
discard_from($$[QT_HOST_DATA/get]/mkspecs/qconfig.pri)
discard_from($$[QT_HOST_DATA/get]/mkspecs/qmodule.pri)
# ... and cause them to be reloaded afterwards
QMAKE_POST_CONFIGURE += \
    "include(\$\$[QT_HOST_DATA/get]/mkspecs/qconfig.pri)" \
    "include(\$\$[QT_HOST_DATA/get]/mkspecs/qmodule.pri)"

# load and process input from configure.sh/.exe
include($$shadowed($$PWD)/config.tests/configure.cfg)
