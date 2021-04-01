#####################################################################
## testProcessEchoGui Binary:
#####################################################################

add_executable(testProcessEchoGui WIN32 main_win.cpp)
target_link_libraries(testProcessEchoGui PRIVATE user32)
