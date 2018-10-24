#####################################################################
## Test Module:
#####################################################################

add_qt_module(Test
    # Typically the Qt module name results in qt${module}-config.h, but
    # testlib is an exception where QtTest uses qttestlib-config.h.
    CONFIG_MODULE_NAME "testlib"
    SOURCES
        qabstracttestlogger.cpp
        qasciikey.cpp
        qbenchmark.cpp qbenchmark.h qbenchmark_p.h
        qbenchmarkevent.cpp qbenchmarkevent_p.h
        qbenchmarkmeasurement.cpp qbenchmarkmeasurement_p.h
        qbenchmarkmetric.cpp qbenchmarkmetric.h qbenchmarkmetric_p.h
        qbenchmarkperfevents.cpp qbenchmarkperfevents_p.h
        qbenchmarktimemeasurers_p.h
        qbenchmarkvalgrind.cpp qbenchmarkvalgrind_p.h
        qcsvbenchmarklogger.cpp
        qplaintestlogger.cpp
        qsignaldumper.cpp
        qsignalspy.h
        qtaptestlogger.cpp qtaptestlogger_p.h
        qteamcitylogger.cpp
        qtest.h
        qtest_gui.h
        qtest_network.h
        qtest_widgets.h
        qtestaccessible.h
        qtestassert.h
        qtestblacklist.cpp qtestblacklist_p.h
        qtestcase.cpp qtestcase.h
        qtestdata.cpp qtestdata.h
        qtestelement.cpp
        qtestelementattribute.cpp
        qtestevent.h
        qtesteventloop.h
        qtesthelpers_p.h
        qtestkeyboard.h
        qtestlog.cpp
        qtestmouse.cpp qtestmouse.h
        qtestresult.cpp
        qtestspontaneevent.h
        qtestsystem.h
        qtesttable.cpp
        qtesttouch.h
        qtestxunitstreamer.cpp
        qttestglobal.h
        qxmltestlogger.cpp
        qxunittestlogger.cpp
    LIBRARIES
        CorePrivate
    PUBLIC_LIBRARIES
        Core
    DEFINES
        QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
        QT_NO_DATASTREAM
)

# Typically the module name is also the define symbol, but testlib works
# differently and uses QT_BUILD_TESTLIB_LIB, derived from the name testlib.pro.
set_target_properties(Test PROPERTIES DEFINE_SYMBOL QT_BUILD_TESTLIB_LIB)

### Platform support:
#####################################################################
extend_target(Test CONDITION APPLE
    SOURCES
        qappletestlogger.cpp qappletestlogger_p.h
    LIBRARIES ${FWSecurity}

    # FIXME: Skip XCTEST support, disabled in qmake build system
)

extend_target(Test CONDITION APPLE_OSX
    SOURCES qtestutil_macos.mm qtestutil_macos_p.h
    LIBRARIES ${FWFoundation} ${FWApplicationServices} ${FWIOKit}
)


### Feature support:
#####################################################################

## itemmodeltester:
extend_target(Test CONDITION QT_FEATURE_itemmodeltester
    SOURCES qabstractitemmodeltester.cpp qabstractitemmodeltester.h
)

add_qt_docs(./doc/qttestlib.qdocconf)
