load(qttest_p4)

SOURCES = tst_qplugin.cpp
QT = core

wince*: {
   plugins.files = plugins/*
   plugins.path = plugins
   DEPLOYMENT += plugins
}

symbian: {
    rpDep.files = releaseplugin.dll debugplugin.dll
    rpDep.path = plugins
    DEPLOYMENT += rpDep dpDep
}
