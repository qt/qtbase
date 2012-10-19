SUBPROGRAMS = \
          testProcessCrash \
          testProcessEcho \
          testProcessEcho2 \
          testProcessEcho3 \
          testProcessEnvironment \
          testProcessNormal \
          testProcessOutput \
          testProcessDeadWhileReading \
          testProcessEOF \
          testExitCodes \
          testGuiProcess \
          testDetached \
          fileWriterProcess \
          testSetWorkingDirectory \
          testSoftExit

!contains(QMAKE_PLATFORM, wince): SUBPROGRAMS += testForwarding

contains(QT_CONFIG, no-widgets): SUBPROGRAMS -= \
          testGuiProcess
