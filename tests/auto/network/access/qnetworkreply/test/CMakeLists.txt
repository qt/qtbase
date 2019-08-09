# Generated from test.pro.

#####################################################################
## tst_qnetworkreply Test:
#####################################################################

# Collect test data
list(APPEND test_data "../empty")
list(APPEND test_data "../rfc3252.txt")
list(APPEND test_data "../resource")
list(APPEND test_data "../bigfile")

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "../*.jpg")
list(APPEND test_data ${test_data_glob})
list(APPEND test_data "../certs")
list(APPEND test_data "../index.html")
list(APPEND test_data "../smb-file.txt")

add_qt_test(tst_qnetworkreply
    GUI
    SOURCES
        ../../../../../shared/emulationdetector.h
        ../tst_qnetworkreply.cpp
    INCLUDE_DIRECTORIES
        ../../../../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

# Resources:
set_source_files_properties("../resource"
    PROPERTIES alias "resource"
)
add_qt_resource(tst_qnetworkreply "qnetworkreply"
    PREFIX
        "/"
    BASE
        ".."
    FILES
        resource
)


#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# CONFIG = "testcase" "-debug_and_release_target" "unsupported/testserver"
# QT_FOR_CONFIG = "gui-private"
# QT_TEST_SERVER_LIST = "vsftpd" "apache2" "ftp-proxy" "danted" "squid"
# testcase.timeout = "600"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:test.pro:NOT ANDROID AND NOT WINRT:
# TEST_HELPER_INSTALLS = "../echo/echo"
