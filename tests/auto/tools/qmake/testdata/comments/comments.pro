LIST = 1 2 3 4 #a comment
!equals( LIST, 1 2 3 4 ) {
   message( "FAILED: inline comment" )
}

LIST = 1 \
       2 \
#      3 \
       4
!equals( LIST, 1 2 4 ) {
   message( "FAILED: commented out continuation" )
}

LIST = 1 \
       2 \#comment
       3 \
       4
!equals( LIST, 1 2 3 4 ) {
    message( "FAILED: comment at end of continuation")
}


LIST = 1 2 3 4#comment
!equals( LIST, 1 2 3 4 ) {
    message( "FAILED: no space before comment" )
}

LIST = 1 2 3 4$${LITERAL_HASH}five
!equals( LIST, 1 2 3 4$${LITERAL_HASH}five ) {
    message( "FAILED: using LITERAL_HASH" )
}
