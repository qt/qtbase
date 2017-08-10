# custom tests

defineReplace(filterLibraryPath) {
    str = $${1}
    for (l, QMAKE_DEFAULT_LIBDIRS): \
        str -= "-L$$l"

    return($$str)
}

defineTest(qtConfLibrary_psqlConfig) {
    pg_config = $$config.input.psql_config
    isEmpty(pg_config): \
        pg_config = $$qtConfFindInPath("pg_config")
    !win32:!isEmpty(pg_config) {
        qtRunLoggedCommand("$$pg_config --libdir", libdir)|return(false)
        qtRunLoggedCommand("$$pg_config --includedir", includedir)|return(false)
        libdir -= $$QMAKE_DEFAULT_LIBDIRS
        libs =
        !isEmpty(libdir): libs += "-L$$libdir"
        libs += "-lpq"
        $${1}.libs = "$$val_escape(libs)"
        includedir -= $$QMAKE_DEFAULT_INCDIRS
        $${1}.includedir = "$$val_escape(includedir)"
        export($${1}.libs)
        export($${1}.includedir)
        return(true)
    }
    qtLog("pg_config not found.")
    return(false)
}

defineTest(qtConfLibrary_psqlEnv) {
    # Respect PSQL_LIBS if set
    PSQL_LIBS = $$getenv(PSQL_LIBS)
    !isEmpty(PSQL_LIBS) {
        $${1}.libs = $$PSQL_LIBS
        export($${1}.libs)
    } else {
        !qtConfLibrary_inline($$1, $$2): \
            return(false)
    }
    return(true)
}

defineTest(qtConfLibrary_mysqlConfig) {
    mysql_config = $$config.input.mysql_config
    isEmpty(mysql_config): \
        mysql_config = $$qtConfFindInPath("mysql_config")
    !isEmpty(mysql_config) {
        qtRunLoggedCommand("$$mysql_config --version", version)|return(false)
        version = $$split(version, '.')
        version = $$first(version)
        isEmpty(version)|lessThan(version, 4): return(false)]

        # query is either --libs or --libs_r
        query = $$eval($${1}.query)
        qtRunLoggedCommand("$$mysql_config $$query", libs)|return(false)
        qtRunLoggedCommand("$$mysql_config --include", includedir)|return(false)
        eval(libs = $$libs)
        libs = $$filterLibraryPath($$libs)
        # -rdynamic should not be returned by mysql_config, but is on RHEL 6.6
        libs -= -rdynamic
        equals($${1}.cleanlibs, true) {
            for(l, libs) {
                # Drop all options besides the -L one and the -lmysqlclient one
                # so we don't unnecessarily link to libs like OpenSSL
                contains(l, "^(-L|-lmysqlclient).*"): cleanlibs += $$l
            }
            libs = $$cleanlibs
        }
        $${1}.libs = "$$val_escape(libs)"
        eval(rawincludedir = $$includedir)
        rawincludedir ~= s/^-I//g
        includedir =
        for (id, rawincludedir): \
            includedir += $$clean_path($$id)
        includedir -= $$QMAKE_DEFAULT_INCDIRS
        $${1}.includedir = "$$val_escape(includedir)"
        export($${1}.libs)
        export($${1}.includedir)
        return(true)
    }
    qtLog("mysql_config not found.")
    return(false)
}

defineTest(qtConfLibrary_sybaseEnv) {
    libs =
    sybase = $$getenv(SYBASE)
    !isEmpty(sybase): \
        libs += "-L$${sybase}/lib"
    libs += $$getenv(SYBASE_LIBS)
    !isEmpty(libs) {
        $${1}.libs = "$$val_escape(libs)"
        export($${1}.libs)
    }
    return(true)
}
