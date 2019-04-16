# custom tests

defineTest(qtConfLibrary_psqlConfig) {
    pg_config = $$config.input.psql_config
    isEmpty(pg_config):!cross_compile: \
        pg_config = $$qtConfFindInPath("pg_config")
    !win32:!isEmpty(pg_config) {
        qtRunLoggedCommand("$$pg_config --libdir", libdir)|return(false)
        !qtConfResolvePathLibs($${1}.libs, $$libdir, -lpq): \
            return(false)
        qtRunLoggedCommand("$$pg_config --includedir", includedir)|return(false)
        !qtConfResolvePathIncs($${1}.includedir, $$includedir, $$2): \
            return(false)
        return(true)
    }
    qtLog("pg_config not found.")
    return(false)
}

defineTest(qtConfLibrary_psqlEnv) {
    # Respect PSQL_LIBS if set
    PSQL_LIBS = $$getenv(PSQL_LIBS)
    !isEmpty(PSQL_LIBS) {
        eval(libs = $$PSQL_LIBS)
        !qtConfResolveLibs($${1}.libs, $$libs): \
            return(false)
    } else {
        !qtConfLibrary_inline($$1, $$2): \
            return(false)
    }
    return(true)
}

defineTest(qtConfLibrary_mysqlConfig) {
    mysql_config = $$config.input.mysql_config
    isEmpty(mysql_config):!cross_compile: \
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
        !qtConfResolveLibs($${1}.libs, $$libs): \
            return(false)
        eval(rawincludedir = $$includedir)
        rawincludedir ~= s/^-I//g
        includedir =
        for (id, rawincludedir): \
            includedir += $$clean_path($$id)
        !qtConfResolvePathIncs($${1}.includedir, $$includedir, $$2): \
            return(false)
        return(true)
    }
    qtLog("mysql_config not found.")
    return(false)
}

defineTest(qtConfLibrary_sybaseEnv) {
    libdir =
    sybase = $$getenv(SYBASE)
    !isEmpty(sybase): \
        libdir += $${sybase}/lib
    eval(libs = $$getenv(SYBASE_LIBS))
    isEmpty(libs): \
        libs = $$eval($${1}.libs)
    !qtConfResolvePathLibs($${1}.libs, $$libdir, $$libs): \
        return(false)
    return(true)
}
