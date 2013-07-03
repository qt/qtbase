VAR = 1 2 3 4 5
JOINEDVAR = $$join( VAR, "-GLUE-", "-BEFORE-", "-AFTER-" )
!contains( JOINEDVAR, -BEFORE-1-GLUE-2-GLUE-3-GLUE-4-GLUE-5-AFTER- ) {
   message( "FAILED: join [$$JOINEDVAR != -BEFORE-1-GLUE-2-GLUE-3-GLUE-4-GLUE-5-AFTER-]" )
}

# To test member we need to use join
NEWVAR = $$member( VAR, 4 ) $$member( VAR, 3 ) $$member( VAR, 2 )
JOINEDNEWVAR = $$join( NEWVAR, "-" )
!contains( JOINEDNEWVAR, 5-4-3 ) {
  message( "FAILED: member [$$JOINEDNEWVAR != 5-4-3]" )
}
