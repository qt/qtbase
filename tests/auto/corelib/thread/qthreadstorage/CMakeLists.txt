# special case skip regeneration
# Generated from qthreadstorage.pro.

#####################################################################
## tst_qthreadstorage Test:
#####################################################################

qt_internal_add_test(tst_qthreadstorage
    SOURCES
        tst_qthreadstorage.cpp
)

## Scopes:
#####################################################################

if(NOT ANDROID)
    add_subdirectory(crashonexit)
endif()
