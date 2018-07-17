CONFIG += testcase

SOURCES += tst_qsslcertificate.cpp
QT = core network testlib

TARGET = tst_qsslcertificate

TESTDATA += certificates/* more-certificates/* verify-certs/* pkcs12/*
