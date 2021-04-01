#####################################################################
## testSoftExit Binary:
#####################################################################

add_executable(testSoftExit)

qt_internal_extend_target(testSoftExit CONDITION WIN32
    SOURCES
        main_win.cpp
    PUBLIC_LIBRARIES
        user32
)

qt_internal_extend_target(testSoftExit CONDITION UNIX
    SOURCES
        main_unix.cpp
)
