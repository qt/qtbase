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
          testForwarding \
          testGuiProcess \
          testDetached \
          fileWriterProcess \
          testSetWorkingDirectory \
          testSoftExit

!qtHaveModule(widgets): SUBPROGRAMS -= \
          testGuiProcess
