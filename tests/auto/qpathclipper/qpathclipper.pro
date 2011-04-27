load(qttest_p4)
INCLUDEPATH += .
HEADERS += paths.h
SOURCES  += tst_qpathclipper.cpp paths.cpp

requires(contains(QT_CONFIG,private_tests))

unix:!mac:!symbian:LIBS+=-lm


