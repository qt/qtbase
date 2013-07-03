defineTest(testVariable) {
    varname=$$1
    value=$$eval($$varname)
    RESULT=$$value
    export(RESULT)
}

defineTest(callTest) {
    myvar=$$1
    testVariable(myvar)
}

defineTest(callTestExport) {
    myvar=$$1
    export(myvar)
    testVariable(myvar)
}

defineTest(callTestExportChange) {
    myvar=foo
    export(myvar)
    myvar=$$1
    testVariable(myvar)
}

value=direct
myvar=$$value
testVariable(myvar)
!isEqual(RESULT,$$value) {
   message( "FAILED: result [$$RESULT] != $$value" )
}

value=export
callTestExport($$value)
!isEqual(RESULT,$$value) {
   message( "FAILED: result [$$RESULT] != $$value" )
}

value=export_and_change
callTestExportChange($$value)
!isEqual(RESULT,$$value) {
   message( "FAILED: result [$$RESULT] != $$value" )
}

value=local
callTest($$value)
!isEqual(RESULT,$$value) {
   message( "FAILED: result [$$RESULT] != $$value" )
}
