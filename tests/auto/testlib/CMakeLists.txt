# Generated from testlib.pro.

# add_subdirectory(outformat) # special case missing project
add_subdirectory(qsignalspy)
# add_subdirectory(selftests) # special case disable because the conversion is broken and hard to fix atm
if(TARGET Qt::Widgets)
    add_subdirectory(qabstractitemmodeltester)
endif()
