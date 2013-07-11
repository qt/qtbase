VAR = qt thread

VAR += debug
!contains( VAR, debug ) {
   message( "FAILED: +=" )
}

VAR -= thread
contains( VAR, thread ) {
   message( "FAILED: -=" )
}

VAR = thread
VAR *= thread
!count( VAR, 1 ) {
   message( "FAILED: *=" )
}

VAR = thread QT_DLL debug
VAR ~= s/QT_+/Q_
!contains( VAR, Q_DLL ) {
   message( "FAILED: ~=" )
}
