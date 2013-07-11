VAR = qt thread

defineTest(testReplace) {
    !isEqual(1, $$2):message("FAILED: $$3: got $$1, expected $${2}.")
}

#count
!count( VAR, 2 ) {
   message( "FAILED: count function: $$VAR" )
}

#contains
!contains( VAR, thread ) {
   message( "FAILED: contains function: $$VAR" )
}

#exists
!exists( functions.pro ) {
   message( "FAILED: exists function" )
}

#isEmpty
isEmpty( VAR ) {
   message( "FAILED: isEmpty function: $VAR" )
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

#recursive
defineReplace(myRecursiveReplaceFunction) {
    RESULT =
    list = $$1
    RESULT += $$member(list, 0)
    list -= $$RESULT
    !isEmpty(list):RESULT += $$myRecursiveReplaceFunction($$list)
    return($$RESULT)
}
RESULT = $$myRecursiveReplaceFunction(oink baa moo)
!isEqual(RESULT, "oink baa moo") {
    message( "FAILED: myRecursiveReplaceFunction [$$RESULT] != oink baa moo" )
}

moo = "this is a test" "for real"
fn = $$OUT_PWD/testdir/afile
write_file($$fn, moo)|message("FAILED: write_file() failed")
exists($$fn)|message("FAILED: write_file() didn't write anything")
mooout = $$cat($$fn, line)
equals(moo, $$mooout)|message("FAILED: write_file() wrote something wrong")
moo += "another line"
write_file($$fn, moo)|message("FAILED: write_file() failed (take 2)")
mooout = $$cat($$fn, line)
equals(moo, $$mooout)|message("FAILED: write_file() wrote something wrong (take 2)")
mooadd = "yet another line"
write_file($$fn, mooadd, append)|message("FAILED: write_file() failed (append)")
moo += $$mooadd
mooout = $$cat($$fn, line)
equals(moo, $$mooout)|message("FAILED: write_file() wrote something wrong when appending")

pn = $$OUT_PWD/testpath/subdir
mkpath($$pn)|message("FAILED: mkpath() failed")
exists($$pn)|message("FAILED: mkpath() didn't create anything")

in = easy "less easy" sca$${LITERAL_HASH}ry crazy$$escape_expand(\\t\\r\\n) $$escape_expand(\\t)shit \'no\"way\\here
out = "easy \"less easy\" sca\$\${LITERAL_HASH}ry crazy\$\$escape_expand(\\\\t\\\\r\\\\n) \$\$escape_expand(\\\\t)shit \\\'no\\\"way\\\\here"
testReplace($$val_escape(in), $$out, "val_escape")

testReplace($$shadowed($$PWD/something), $$OUT_PWD/something, "shadowed")
testReplace($$shadowed($$PWD), $$OUT_PWD, "shadowed (take 2)")

#format_number
spc = " "
testReplace($$format_number(13), 13, "simple number format")
testReplace($$format_number(-13), -13, "negative number format")
testReplace($$format_number(13, ibase=16), 19, "hex input number format")
testReplace($$format_number(13, obase=16), d, "hex output number format")
testReplace($$format_number(13, width=5), " $$spc 13", "right aligned number format")
testReplace($$format_number(13, width=5 leftalign), "13 $$spc ", "left aligned number format")
testReplace($$format_number(13, width=5 zeropad), "00013", "zero-padded number format")
testReplace($$format_number(13, width=5 alwayssign), "$$spc +13", "always signed number format")
testReplace($$format_number(13, width=5 alwayssign zeropad), "+0013", "zero-padded always signed number format")
testReplace($$format_number(13, width=5 padsign), " $$spc 13", "sign-padded number format")
testReplace($$format_number(13, width=5 padsign zeropad), " 0013", "zero-padded sign-padded number format")

testReplace($$clean_path("c:$${DIR_SEPARATOR}crazy//path/../trolls"), "c:/crazy/trolls", "clean_path")

testReplace($$shell_path("/crazy/trolls"), "$${QMAKE_DIR_SEP}crazy$${QMAKE_DIR_SEP}trolls", "shell_path")
testReplace($$system_path("/crazy/trolls"), "$${DIR_SEPARATOR}crazy$${DIR_SEPARATOR}trolls", "system_path")

testReplace($$absolute_path("crazy/trolls"), "$$PWD/crazy/trolls", "absolute_path")
testReplace($$absolute_path("crazy/trolls", "/fake/path"), "/fake/path/crazy/trolls", "absolute_path with base")
testReplace($$absolute_path(""), "$$PWD", "absolute_path of empty")
testReplace($$relative_path($$_PRO_FILE_PWD_), $$basename($$_PRO_FILE_), "relative_path")
testReplace($$relative_path("/fake/trolls", "/fake/path"), "../trolls", "relative_path with base")
testReplace($$relative_path(""), "", "relative_path of empty")

#this test is very rudimentary. the backend function is thoroughly tested in qt creator
in = "some nasty\" path\\"
out_cmd = "\"some nasty\"\\^\"\" path\"\\"
out_sh = "'some nasty\" path\\'"
equals(QMAKE_HOST.os, Windows): \
    out = $$out_cmd
else: \
    out = $$out_sh
testReplace($$system_quote($$in), $$out, "system_quote")
!equals(QMAKE_DIR_SEP, /): \
    out = $$out_cmd
else: \
    out = $$out_sh
testReplace($$shell_quote($$in), $$out, "shell_quote")

testReplace($$reverse($$list(one two three)), three two one, "reverse")

testReplace($$cat(textfile), hi '"holla he"' 'hu!')
