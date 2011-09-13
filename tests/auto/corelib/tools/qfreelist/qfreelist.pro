load(qttest_p4)
SOURCES  += tst_qfreelist.cpp
QT += core-private
QT -= gui
!contains(QT_CONFIG,private_tests): SOURCES += $$QT.core.sources/tools/qfreelist.cpp
