SUBPROGRAMS = \
          testProcessCrash \
          testProcessEcho \
          testProcessEcho2 \
          testProcessEcho3 \
          testProcessEnvironment \
          testProcessHang \
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

!qtHaveModule(widgets): SUBPROGRAMS -= \
          testGuiProcess
