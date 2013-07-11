defineTest(doExport) {
        EXPORTED += $$1
        export(EXPORTED)
}

defineTest(callDoExport) {
        doExport(bar)
        doExport(baz)
        EXPORTED = oink
        !isEqual(EXPORTED, "oink") {
           message( "FAILED: function-scope exports [$$EXPORTED] != oink" )
        }
}

doExport(foo)
callDoExport()
!isEqual(EXPORTED, "foo bar baz") {
   message( "FAILED: global-scope exports [$$EXPORTED] != foo bar baz" )   
}
