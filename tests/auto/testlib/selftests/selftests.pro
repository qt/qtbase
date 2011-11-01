TEMPLATE = subdirs

SUBDIRS = subtest test warnings maxwarnings cmptest globaldata skip \
          strcmp expectfail sleep fetchbogus crashes multiexec failinit failinitdata \
          skipinit skipinitdata datetime singleskip assert differentexec \
          exceptionthrow qexecstringlist datatable commandlinedata\
          benchlibwalltime benchlibcallgrind benchlibeventcounter benchlibtickcounter \
          benchliboptions xunit badxml longstring float printdatatags \
          printdatatagswithglobaltags

INSTALLS =

QT = core


CONFIG += parallel_test
