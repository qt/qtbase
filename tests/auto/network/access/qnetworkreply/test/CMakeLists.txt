# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qnetworkreply Test:
#####################################################################

# Collect test data
list(APPEND test_data "../empty")
list(APPEND test_data "../rfc3252.txt")
list(APPEND test_data "../resource")
list(APPEND test_data "../bigfile")
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ../*.jpg)
list(APPEND test_data ${test_data_glob})
list(APPEND test_data "../certs")
list(APPEND test_data "../index.html")
list(APPEND test_data "../smb-file.txt")

qt_internal_add_test(tst_qnetworkreply
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qnetworkreply.cpp
        ../data/gzip.rcc.cpp
        ../data/zstandard.rcc.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    TESTDATA ${test_data}
    QT_TEST_SERVER_LIST "vsftpd" "apache2" "ftp-proxy" "danted" "squid"
    BUNDLE_ANDROID_OPENSSL_LIBS
)
add_dependencies(tst_qnetworkreply echo)

# Resources:
set(qnetworkreply_resource_files
    "../resource"
)

qt_internal_add_resource(tst_qnetworkreply "qnetworkreply"
    PREFIX
        "/"
    BASE
        ".."
    FILES
        ${qnetworkreply_resource_files}
)
