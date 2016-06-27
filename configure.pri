# custom command line handling

defineTest(qtConfCommandline_cxxstd) {
    arg = $${1}
    val = $${2}
    isEmpty(val): val = $$qtConfGetNextCommandlineArg()
    message("setting c++std: $$arg/$$val")
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
            error("Invalid argument $$val to command line parameter $$arg")
        }
    } else {
        error("Missing argument to command line parameter $$arg")
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
            error("Invalid argument $$val to command line parameter $$arg")
        }
    } else {
        error("Missing argument to command line parameter $$arg")
    }
}

# callbacks

defineReplace(qtConfFunc_crossCompile) {
    spec = $$[QMAKE_SPEC]
    !equals(spec, $$[QMAKE_XSPEC]): return(true)
    return(false)
}

# custom tests

defineTest(qtConfTest_avx_test_apple_clang) {
    !*g++*:!*-clang*: return(true)

    compiler = $$system("$$QMAKE_CXX --version")
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
        version = $$system("$$make -v", blob)
        contains(version, "^GNU Make.*"): return(true)
    }
    return(false)
}

defineTest(qtConfTest_detectPkgConfig) {
    pkgConfig = $$getenv("PKG_CONFIG")
    !isEmpty(pkgConfig): {
        qtLog("Found pkg-config from environment variable: $$pkgConfig")
    } else {
        pkgConfig = $$PKG_CONFIG

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
                gccMachineDump = $$system("$$QMAKE_CXX -dumpmachine")
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
    }
    $${1}.pkgConfig = $$pkgConfig
    export($${1}.pkgConfig)

    PKG_CONFIG = $$pkgConfig
    export(PKG_CONFIG)

    return(true)
}

defineTest(qtConfTest_neon) {
    contains(config.input.cpufeatures, "neon"): return(true)
    return(false)
}

defineTest(qtConfTest_openssl) {
    libs = $$getenv("OPENSSL_LIBS")

    !isEmpty(libs) {
        $${1}.libs = $$libs
        export($${1}.libs)
    }

    $${1}.showNote = false
    isEmpty(libs): $${1}.showNote = true
    export($${1}.showNote)

    return(true)
}

defineTest(qtConfTest_checkCompiler) {
    contains(QMAKE_CXX, ".*clang.*") {
        versionstr = "$$system($$QMAKE_CXX -v 2>&1)"
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
        $${1}.compilerDescription = "GCC"
        $${1}.compilerId = "gcc"
        $${1}.compilerVersion = $$system($$QMAKE_CXX -dumpversion)
    } else: contains(QMAKE_CXX, ".*icpc"  ) {
        $${1}.compilerDescription = "ICC"
        $${1}.compilerId = "icc"
        version = "$$system($$QMAKE_CXX -v)"
        $${1}.compilerVersion = $$replace(version, "icpc version ([0-9.]+).*", "\\1")
    } else: msvc {
        $${1}.compilerDescription = "MSVC"
        $${1}.compilerId = "cl"
    } else {
        return(false)
    }
    $${1}.compilerDescription += $$eval($${1}.compilerVersion)
    export($${1}.compilerDescription)
    export($${1}.compilerId)
    export($${1}.compilerVersion)
    return(true)
}

defineReplace(filterLibraryPath) {
    str = $${1}
    for (l, QMAKE_DEFAULT_LIBDIRS): \
        str -= "-L$$l"

    return($$str)
}

defineTest(qtConfTest_psqlCompile) {
    pg_config = $$config.input.psql_config
    isEmpty(pg_config): \
        pg_config = $$qtConfFindInPath("pg_config")
    !win32:!isEmpty(pg_config) {
        libdir = $$system("$$pg_config --libdir")
        libdir -= $$QMAKE_DEFAULT_LIBDIRS
        !isEmpty(libdir): libs = "-L$$libdir"
        libs += "-lpq"
        $${1}.libs = $$libs
        $${1}.includedir = $$system("$$pg_config --includedir")
        $${1}.includedir -= $$QMAKE_DEFAULT_INCDIRS
        !isEmpty($${1}.includedir): \
            $${1}.cflags = "-I$$eval($${1}.includedir)"
    }

    # Respect PSQL_LIBS if set
    PSQL_LIBS = $$getenv(PSQL_LIBS)
    !isEmpty($$PSQL_LIBS): $${1}.libs = $$PSQL_LIBS

    export($${1}.libs)
    export($${1}.includedir)
    export($${1}.cflags)

    qtConfTest_compile($${1}): return(true)
    return(false)
}

defineTest(qtConfTest_mysqlCompile) {
    mysql_config = $$config.input.mysql_config
    isEmpty(mysql_config): \
        mysql_config = $$qtConfFindInPath("mysql_config")
    !isEmpty(mysql_config) {
        version = $$system("$$mysql_config --version")
        version = $$split(version, '.')
        version = $$first(version)
        isEmpty(version)|lessThan(version, 4): return(false)]

        # query is either --libs or --libs_r
        query = $$eval($${1}.query)
        $${1}.libs = $$filterLibraryPath($$system("$$mysql_config $$query"))
        # -rdynamic should not be returned by mysql_config, but is on RHEL 6.6
        $${1}.libs -= -rdynamic
        includedir = $$system("$$mysql_config --include")
        includedir ~= s/^-I//g
        includedir -= $$QMAKE_DEFAULT_INCDIRS
        $${1}.includedir = $$includedir
        !isEmpty($${1}.includedir): \
            $${1}.cflags = "-I$$eval($${1}.includedir)"
        export($${1}.libs)
        export($${1}.includedir)
        export($${1}.cflags)
    }

    qtConfTest_compile($${1}): return(true)
    return(false)
}

defineTest(qtConfTest_tdsCompile) {
    sybase = $$getenv(SYBASE)
    !isEmpty(sybase): \
        $${1}.libs = "-L$${sybase}/lib"
    $${1}.libs += $$getenv(SYBASE_LIBS)
    export($${1}.libs)

    qtConfTest_compile($${1}): return(true)
    return(false)
}


defineTest(qtConfTest_xkbConfigRoot) {
    qtConfTest_getPkgConfigVariable($${1}): return(true)

    for (dir, $$list("/usr/share/X11/xkb", "/usr/local/share/X11/xkb")) {
        exists($$dir) {
            $${1}.value = $$dir
            export($${1}.value)
            return(true)
        }
    }
    return(false)
}

defineTest(qtConfTest_qpaDefaultPlatform) {
    name =
    !isEmpty(config.input.qpa_default_platform): name = $$config.input.qpa_default_platform
    else: !isEmpty(QT_QPA_DEFAULT_PLATFORM): name = $$QT_QPA_DEFAULT_PLATFORM
    else: win32: name = windows
    else: android: name = android
    else: osx: name = cocoa
    else: ios: name = ios
    else: qnx: name = qnx
    else: integrity: name = integrityfb
    else: name = xcb

    $${1}.value = $$name
    $${1}.plugin = q$$name
    $${1}.name = "\"$$name\""
    export($${1}.value)
    export($${1}.plugin)
    export($${1}.name)
    return(true)
}


# custom outputs

defineTest(qtConfOutput_shared) {
    !$${2}: return()

    # export this here, so later tests can use it
    CONFIG += shared
    export(CONFIG)
}

defineTest(qtConfOutput_verbose) {
    !$${2}: return()

    qtConfOutputVar(assign, "privatePro", "QMAKE_CONFIG_VERBOSE", true)

    # export this here, so we can get verbose logging
    QMAKE_CONFIG_VERBOSE = true
    export(QMAKE_CONFIG_VERBOSE)
}

defineTest(qtConfOutput_styles) {
    !$${2}: return()

    style = $$replace($${1}.feature, "-style", "")
    qtConfOutputVar(append, "privatePro", "styles", $$style)
}

defineTest(qtConfOutput_sqldriver) {
    $${2}: qtConfOutputVar(append, "privatePro", "sql-drivers", $$eval($${1}.feature))
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

    # this method also exports PKG_CONFIG_(LIB|SYSROOT)DIR, so that tests using pkgConfig will work correctly
    PKG_CONFIG_SYSROOT_DIR = $$eval(config.tests.pkg-config.pkgConfigSysrootDir)
    !isEmpty(PKG_CONFIG_SYSROOT_DIR) {
        qtConfOutputVar(assign, "publicPro", "PKG_CONFIG_SYSROOT_DIR", $$PKG_CONFIG_SYSROOT_DIR)
        export(PKG_CONFIG_SYSROOT_DIR)
    }
    PKG_CONFIG_LIBDIR = $$eval(config.tests.pkg-config.pkgConfigLibdir)
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

    name = $$upper($$config.tests.compiler.compilerId)
    version = $$config.tests.compiler.compilerVersion
    major = $$section(version, '.', 0, 0)
    minor = $$section(version, '.', 1, 1)
    patch = $$section(version, '.', 2, 2)
    isEmpty(minor): minor = 0
    isEmpty(patch): patch = 0

    config.output.publicPro += \
        "QT_$${name}_MAJOR_VERSION = $$major" \
        "QT_$${name}_MINOR_VERSION = $$minor" \
        "QT_$${name}_PATCH_VERSION = $$patch"

    export(config.output.publicPro)
}

# should go away when qfeatures.txt is ported
defineTest(qtConfOutput_extraFeatures) {
    isEmpty(config.input.extra_features): return()

    # write to qconfig.pri
    config.output.publicPro += "$${LITERAL_HASH}ifndef QT_BOOTSTRAPPED"
    for (f, config.input.extra_features) {
        feature = $$replace(f, "^no-", "")
        FEATURE = $$upper($$replace(feature, -, _))
        contains(f, "^no-.*") {
            config.output.publicPro += \
                "$${LITERAL_HASH}ifndef QT_NO_$$FEATURE" \
                "$${LITERAL_HASH}define QT_NO_$$FEATURE" \
                "$${LITERAL_HASH}endif"
        } else {
            config.output.publicPro += \
                "$${LITERAL_HASH}if defined(QT_$$FEATURE) && defined(QT_NO_$$FEATURE)" \
                "$${LITERAL_HASH}undef QT_$$FEATURE" \
                "$${LITERAL_HASH}elif !defined(QT_$$FEATURE) && !defined(QT_NO_$$FEATURE)" \
                "$${LITERAL_HASH}define QT_$$FEATURE" \
                "$${LITERAL_HASH}endif"
        }
    }
    config.output.publicPro += "$${LITERAL_HASH}endif"
    export(config.output.publicPro)

    # write to qmodule.pri
    disabled_features =
    for (f, config.input.extra_features) {
        feature = $$replace(f, "^no-", "")
        FEATURE = $$upper($$replace(feature, -, _))
        contains(f, "^no-.*"): disabled_features += $$FEATURE
    }
    !isEmpty(disabled_features): qtConfOutputVar(assign, "privatePro", QT_NO_DEFINES, $$disabled_features)
}


defineTest(qtConfOutputPostProcess_privatePro) {
    output = \
        "host_build {" \
        "    QT_CPU_FEATURES.$$QT_HOST_ARCH = $$config.input.host_cpufeatures" \
        "} else {" \
        "    QT_CPU_FEATURES.$$QT_ARCH = $$config.input.cpufeatures" \
        "}"

    output += $$cat($$OUT_PWD/.qmake.vars, lines)

    config.output.privatePro += $$output
    export(config.output.privatePro)
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
        output += "EXTRA_DEFINES += $$config.input.defines"
    }
    !isEmpty(config.input.includes) {
        EXTRA_INCLUDEPATH += $$config.input.includes
        export(EXTRA_INCLUDEPATH)
        output += "EXTRA_INCLUDEPATH += $$config.input.includes"
    }

    libs = $$join(config.input.lpaths, " -L", "-L")
    libs += $$join(config.input.libs, " -l", "-l")

    darwin {
        libs += $$join(config.input.fpaths, " -F", "-F")
        libs += $$join(config.input.frameworks, " -framework ", "-framework ")
    }

    !isEmpty(libs) {
        EXTRA_LIBS += $$libs
        export(EXTRA_LIBS)

        output += "EXTRA_LIBS += $$libs"
    }

    config.output.privatePro += $$output
    export(config.output.privatePro)
}

defineTest(qtConfOutput_gccSysroot) {
    !$${2}: return()

    # This variable also needs to be exported immediately, so the compilation tests
    # can pick it up.
    EXTRA_QMAKE_ARGS = \
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
    config.output.publicPro += $$output
    export(config.output.publicPro)
}

defineTest(qtConfOutputPostProcess_publicPro) {
    qt_version = $$[QT_VERSION]
    output = \
        "QT_VERSION = $$qt_version" \
        "QT_MAJOR_VERSION = $$section(qt_version, '.', 0, 0)" \
        "QT_MINOR_VERSION = $$section(qt_version, '.', 1, 1)" \
        "QT_PATCH_VERSION = $$section(qt_version, '.', 2, 2)" \
        \
        "host_build {" \
        "    QT_ARCH = $$QT_HOST_ARCH" \
        "    QT_TARGET_ARCH = $$QT_ARCH" \
        "} else {" \
        "    QT_ARCH = $$QT_ARCH" \
        "}"

    #libinfix and namespace
    !isEmpty(config.input.qt_libinfix): output += "QT_LIBINFIX = $$config.input.qt_libinfix"
    !isEmpty(config.input.qt_namespace): output += "QT_NAMESPACE = $$config.input.qt_namespace"

    output += "QT_EDITION = $$config.input.qt_edition"
    !contains(config.input.qt_edition, "(OpenSource|Preview)") {
        output += \
            "QT_LICHECK = $$config.input.qt_licheck" \
            "QT_RELEASE_DATE = $$config.input.qt_release_date"
    }

    config.output.publicPro += $$output
    export(config.output.publicPro)
}

defineTest(qtConfOutputPostProcess_publicHeader) {
    qt_version = $$[QT_VERSION]
    output = \
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

    config.output.publicHeader += $$output
    export(config.output.publicHeader)
}


# custom reporting

defineTest(qtConfReport_buildParts) {
    qtConfReportPadded($${1}, $$config.input.qt_build_parts)
}

defineTest(qtConfReport_buildTypeAndConfig) {
    equals(QT_ARCH, $$QT_HOST_ARCH) {
        qtConfAddReport("Build type: $$QT_ARCH")
    } else {
        qtConfAddReport("Building on:  $$QT_HOST_ARCH")
        qtConfAddReport("Building for: $$QT_ARCH")
    }
    qtConfAddReport()
    qtConfAddReport("Configuration: $$config.output.privatePro.append.CONFIG $$config.output.publicPro.append.QT_CONFIG")
    qtConfAddReport()
}

defineTest(qtConfReport_buildMode) {
    $$qtConfEvaluate("features.force_debug_info"): \
        release = "release (with debug info)"
    else: \
        release = "release"

    $$qtConfEvaluate("features.debug"): \
        build_mode = "debug"
    else: \
        build_mode = $$release

    $$qtConfEvaluate("features.debug_and_release"): \
        build_mode = "debug and $$release; default link: $$build_mode"

    $$qtConfEvaluate("features.release_tools"): \
        build_mode = "$$build_mode; optimized tools"

    qtConfReportPadded($$1, $$build_mode)
}

# load and process input from configure
exists("$$OUT_PWD/config.tests/configure.cfg") {
    include("$$OUT_PWD/config.tests/configure.cfg")
}

load(qt_configure)
