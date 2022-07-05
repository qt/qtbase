# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_qprocess Test:
#####################################################################

qt_internal_add_test(tst_qprocess
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qprocess.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::TestPrivate
)

## Build assorted sub-programs called from the test:
add_dependencies(tst_qprocess
    testProcessCrash
    testProcessEcho
    testProcessEcho2
    testProcessEcho3
    testProcessEnvironment
    testProcessHang
    testProcessNormal
    testProcessOutput
    testProcessDeadWhileReading
    testProcessEOF
    testExitCodes
    testForwarding
    testDetached
    fileWriterProcess
    testSetWorkingDirectory
    testSoftExit
    nospace onespace twospaces
    testSpaceInName
)
if(TARGET Qt::Widgets)
    add_dependencies(tst_qprocess testGuiProcess)
endif()
if(WIN32)
    add_dependencies(tst_qprocess testProcessEchoGui testSetNamedPipeHandleState)
endif()

#### Keys ignored in scope 2:.:.:test.pro:WIN32:
# TESTDATA = "../testBatFiles/*"
