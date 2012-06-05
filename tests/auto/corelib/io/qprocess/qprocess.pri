SUBPROGRAMS = \
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
          testExitCodes \
          testGuiProcess \
          testDetached \
          fileWriterProcess \
          testSetWorkingDirectory \
          testSoftExit

contains(QT_CONFIG, no-widgets): SUBPROGRAMS -= \
          testGuiProcess
