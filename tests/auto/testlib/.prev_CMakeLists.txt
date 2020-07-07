# Generated from testlib.pro.

add_subdirectory(initmain)
add_subdirectory(outformat)
add_subdirectory(qsignalspy)
add_subdirectory(selftests)
if(TARGET Qt::Widgets)
    add_subdirectory(qabstractitemmodeltester)
endif()
