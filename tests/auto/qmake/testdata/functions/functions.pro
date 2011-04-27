CONFIG = qt thread

#count
!count( CONFIG, 2 ) {
   message( "FAILED: count function: $$CONFIG" )
}

#contains
!contains( CONFIG, thread ) {
   message( "FAILED: contains function: $$CONFIG" )
}

#exists
!exists( functions.pro ) {
   message( "FAILED: exists function" )
}

#isEmpty
isEmpty( CONFIG ) {
   message( "FAILED: isEmpty function: $CONFIG" )
}

#files
!equals($$list($$files(one/*.cpp)), "one/1.cpp one/2.cpp") {
   message( "FAILED: files function: one/*.cpp" )
}
!equals($$list($$files(one/1*.cpp)), "one/1.cpp") {
   message( "FAILED: files function: one/1*.cpp" )
}
!equals($$list($$files(two/*.cpp)), "two/1.cpp two/2.cpp") {
   message( "FAILED: files function: two/*.cpp" )
}
!equals($$list($$files(three/wildcard*.cpp)), "three/wildcard21.cpp three/wildcard22.cpp") {
   message( "FAILED: files function: three/wildcard*.cpp" )
}
!equals($$list($$files(*.cpp)), "1.cpp 2.cpp wildcard21.cpp wildcard22.cpp") {
   message( "FAILED: files function: *.cpp" )
}
!equals($$list($$files(wildcard*.cpp)), "wildcard21.cpp wildcard22.cpp") {
   message( "FAILED: files function: wildcard*.cpp" )
}

#infile
!infile( infiletest.pro, DEFINES, QT_DLL ){
   message( "FAILED: infile function" )
}

#include
include( infiletest.pro, "", true ) 
!contains( DEFINES, QT_DLL ) {
   message( "FAILED: include function: $$DEFINES" )
}

#replace
VERSION=1.0.0
VERSION_replaced=$$replace(VERSION,\\.,_)
!isEqual(VERSION_replaced, 1_0_0) {
   message( "FAILED: replace function: $$VERSION_replaced" )
}

#test functions
defineTest(myTestFunction) {
   RESULT =
   list=$$1
   for(l, list) {
       RESULT += $$l
   }
   export(RESULT)
}
myTestFunction(oink baa moo)
!equals($$list($$member(RESULT, 0)), "oink") {
     message("FAILED: myTestFunction: $$RESULT")
}
myTestFunction("oink baa" moo)
!equals($$list($$member(RESULT, 0)), "oink baa") {
     message("FAILED: myTestFunction: $$RESULT")
}
myTestFunction(oink "baa moo")
!equals($$list($$member(RESULT, 0)), "oink") {
     message("FAILED: myTestFunction: $$RESULT")
}
myTestFunction("oink baa moo")
!equals($$list($$member(RESULT, 0)), "oink baa moo") {
     message("FAILED: myTestFunction: $$RESULT")
}


