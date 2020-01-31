# Generated from test.pro.

#####################################################################
## tst_qtcpsocket Test:
#####################################################################

add_qt_test(tst_qtcpsocket
    SOURCES
        ../tst_qtcpsocket.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

## Scopes:
#####################################################################

extend_target(tst_qtcpsocket CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)

# special case begin
set(THREADS_PREFER_PTHREAD_FLAG 1)
find_package(Threads)
target_link_libraries(tst_qtcpsocket PRIVATE Threads::Threads)
# special case end

#### Keys ignored in scope 4:.:.:test.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug"

#### Keys ignored in scope 5:.:.:test.pro:else:
# DESTDIR = "../release"

#### Keys ignored in scope 6:.:.:test.pro:else:
# DESTDIR = "../"

#### Keys ignored in scope 7:.:.:test.pro:LINUX:
# QT_TEST_SERVER_LIST = "danted" "squid" "apache2" "ftp-proxy" "vsftpd" "iptables" "cyrus"
