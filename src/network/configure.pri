# custom tests

defineTest(qtConfLibrary_openssl) {
    libs = $$getenv("OPENSSL_LIBS")
    !isEmpty(libs) {
        $${1}.libs = $$libs
        export($${1}.libs)
        return(true)
    }
    qtLog("$OPENSSL_LIBS is not set.")
    return(false)
}

