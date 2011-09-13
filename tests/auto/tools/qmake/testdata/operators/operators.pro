CONFIG = qt thread

CONFIG += debug
!contains( CONFIG, debug ) {
   message( "FAILED: +=" )
}

CONFIG -= thread
contains( CONFIG, thread ) {
   message( "FAILED: -=" )
}

CONFIG = thread
CONFIG *= thread
!count( CONFIG, 1 ) {
   message( "FAILED: *=" )
}

CONFIG = thread QT_DLL debug
CONFIG ~= s/QT_+/Q_
!contains( CONFIG, Q_DLL ) {
   message( "FAILED: ~=" )
}

