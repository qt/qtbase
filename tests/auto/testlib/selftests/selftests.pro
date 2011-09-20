TEMPLATE = subdirs

SUBDIRS = subtest test warnings maxwarnings cmptest globaldata skipglobal skip \
          strcmp expectfail sleep fetchbogus crashes multiexec failinit failinitdata \
          skipinit skipinitdata datetime singleskip assert differentexec \
          exceptionthrow qexecstringlist datatable commandlinedata\
          benchlibwalltime benchlibcallgrind benchlibeventcounter benchlibtickcounter \
          benchliboptions xunit badxml longstring

INSTALLS =

QT = core


CONFIG += parallel_test
