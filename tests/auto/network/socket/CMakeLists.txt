# Generated from socket.pro.

if(QT_FEATURE_private_tests)
    add_subdirectory(qhttpsocketengine)
    add_subdirectory(qtcpsocket)
    add_subdirectory(qsocks5socketengine)
    add_subdirectory(platformsocketengine)
endif()
add_subdirectory(qudpsocket)
add_subdirectory(qabstractsocket)
if(NOT ANDROID)
    # QTBUG-87387 # special case
    add_subdirectory(qlocalsocket)
    # QTBUG-87388 # special case
    add_subdirectory(qtcpserver)
endif()
if(QT_FEATURE_sctp)
    add_subdirectory(qsctpsocket)
endif()
