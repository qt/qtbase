CONFIG = 1 2 3 4 5
JOINEDCONFIG = $$join( CONFIG, "-GLUE-", "-BEFORE-", "-AFTER-" )
!contains( JOINEDCONFIG, -BEFORE-1-GLUE-2-GLUE-3-GLUE-4-GLUE-5-AFTER- ) {
   message( "FAILED: join [$$JOINEDCONFIG != -BEFORE-1-GLUE-2-GLUE-3-GLUE-4-GLUE-5-AFTER-]" )
}

# To test member we need to use join
NEWCONFIG = $$member( CONFIG, 4 ) $$member( CONFIG, 3 ) $$member( CONFIG, 2 ) 
JOINEDNEWCONFIG = $$join( NEWCONFIG, "-" )
!contains( JOINEDNEWCONFIG, 5-4-3 ) {
  message( "FAILED: member [$$JOINEDNEWCONFIG != 5-4-3]" )
}


