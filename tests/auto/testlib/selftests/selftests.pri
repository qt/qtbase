SUBPROGRAMS = \
     assert \
     badxml \
     benchlibcallgrind \
     benchlibcounting \
     benchlibeventcounter \
     benchliboptions \
     benchlibtickcounter \
     benchlibwalltime \
     blacklisted \
     cmptest \
     commandlinedata \
     counting \
     crashes \
     datatable \
     datetime \
     deleteLater \
     deleteLater_noApp \
     differentexec \
     exceptionthrow \
     expectfail \
     failcleanup \
     faildatatype \
     failfetchtype \
     failinit \
     failinitdata \
     fetchbogus \
     findtestdata \
     float \
     globaldata \
     longstring \
     maxwarnings \
     multiexec \
     pass \
     pairdiagnostics \
     printdatatags \
     printdatatagswithglobaltags \
     qexecstringlist \
     silent \
     signaldumper \
     singleskip \
     skip \
     skipcleanup \
     skipinit \
     skipinitdata \
     sleep \
     strcmp \
     subtest \
     testlib \
     tuplediagnostics \
     verbose1 \
     verbose2 \
     verifyexceptionthrown \
     warnings \
     watchdog \
     xunit

qtHaveModule(gui): SUBPROGRAMS += \
    keyboard \
    mouse

INCLUDEPATH += ../../../../shared/
HEADERS += ../../../../shared/emulationdetector.h
