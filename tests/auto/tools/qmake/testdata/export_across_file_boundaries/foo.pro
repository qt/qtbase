!equals(FOO,bar) {
   message( "FAILED: export() invisible from default_pre.prf to foo.pro" )
}

O_FOO=$$fromfile(oink.pri,FOO)
O_BAR=$$fromfile(oink.pri,BAR)

!equals(O_BAR,bar) {
   message( "FAILED: export() invisible from oink.pri through \$$fromfile()" )
}

!equals(O_FOO,bar) {
   message( "FAILED: export() invisible from default_pre.prf through \$$fromfile()" )
}
