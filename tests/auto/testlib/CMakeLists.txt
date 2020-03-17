# Generated from testlib.pro.

# add_subdirectory(outformat) # special case missing project
add_subdirectory(qsignalspy)
add_subdirectory(selftests)
if(TARGET Qt::Widgets)
    add_subdirectory(qabstractitemmodeltester)
endif()
