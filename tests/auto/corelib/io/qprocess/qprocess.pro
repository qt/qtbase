TEMPLATE = subdirs

SUBDIRS = \
          testProcessCrash \
          testProcessEcho \
          testProcessEcho2 \
          testProcessEcho3 \
          testProcessEnvironment \
          testProcessLoopback \
          testProcessNormal \
          testProcessOutput \
          testProcessDeadWhileReading \
          testProcessEOF \
          testProcessSpacesArgs/nospace.pro \
          testExitCodes \
          testSpaceInName \
          testGuiProcess \
          testDetached \
          fileWriterProcess \
          testSetWorkingDirectory

!symbian: {
SUBDIRS +=testProcessSpacesArgs/onespace.pro \
          testProcessSpacesArgs/twospaces.pro \
          testSoftExit
}

win32:!wince*:SUBDIRS+=testProcessEchoGui

SUBDIRS += test


