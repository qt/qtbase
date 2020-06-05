# special case skip regeneration
# Generated from qthreadstorage.pro.

#####################################################################
## tst_qthreadstorage Test:
#####################################################################

add_qt_test(tst_qthreadstorage
    SOURCES
        tst_qthreadstorage.cpp
    LIBRARIES # special case
        Threads::Threads # special case

)

## Scopes:
#####################################################################

if(NOT ANDROID)
    add_subdirectory(crashonexit)
endif()
