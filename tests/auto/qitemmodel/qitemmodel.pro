load(qttest_p4)
SOURCES         += tst_qitemmodel.cpp

QT += sql

# NOTE: The deployment of the sqldrivers is disabled on purpose.
#       If we deploy the plugins, they are loaded twice when running
#       the tests on the autotest system. In that case we run out of
#       memory on Windows Mobile 5.

#wince*: {
#   plugFiles.files = $$QT_BUILD_TREE/plugins/sqldrivers/*.dll
#   plugFiles.path    = sqldrivers
#   DEPLOYMENT += plugFiles 
#}

symbian {
    TARGET.EPOCHEAPSIZE="0x100000 0x1000000" # // Min 1Mb, max 16Mb
    qt_not_deployed {
        contains(S60_VERSION, 3.1)|contains(S60_VERSION, 3.2)|contains(S60_VERSION, 5.0) {
            sqlite.path = /sys/bin
            sqlite.files = sqlite3.dll
            DEPLOYMENT += sqlite
        }
    }
}
