# Generated from socket.pro.

add_subdirectory(qudpsocket)
### add_subdirectory(qlocalsocket)
add_subdirectory(qtcpserver)
add_subdirectory(qabstractsocket)

if(QT_FEATURE_private_tests)
    add_subdirectory(platformsocketengine)
    add_subdirectory(qtcpsocket)

    if(NOT WINRT)
        add_subdirectory(qhttpsocketengine)
        add_subdirectory(qsocks5socketengine)
    endif()
endif()

if(QT_FEATURE_sctp)
    add_subdirectory(qsctpsocket)
endif()
