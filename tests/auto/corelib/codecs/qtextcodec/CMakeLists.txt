add_qt_test(tst_qtextcodec SOURCES tst_qtextcodec.cpp)

if(UNIX)
    add_subdirectory(echo)
endif()
