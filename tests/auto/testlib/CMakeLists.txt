# Generated from testlib.pro.

# add_subdirectory(initmain) # special case missing project
# add_subdirectory(outformat) # special case missing project
add_subdirectory(qsignalspy)
# QTBUG-88507 # special case
if(QT_FEATURE_process AND NOT ANDROID)
    add_subdirectory(selftests)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(qabstractitemmodeltester)
endif()
