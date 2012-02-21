CONFIG += testcase
QT -= gui
SOURCES  += ../tst_qnetworkreply.cpp
TARGET = ../tst_qnetworkreply

contains(QT_CONFIG,xcb): CONFIG+=insignificant_test  # unstable, QTBUG-21102

QT = core-private network-private testlib
RESOURCES += ../qnetworkreply.qrc

contains(QT_CONFIG,ipv6ifname): DEFINES += HAVE_IPV6
TESTDATA += ../empty ../rfc3252.txt ../resource ../bigfile ../*.jpg ../certs \
            ../index.html ../smb-file.txt

win32:CONFIG += insignificant_test # QTBUG-24226
load(testcase) # for target.path and installTestHelperApp()
installTestHelperApp("../echo/echo",echo,echo)
