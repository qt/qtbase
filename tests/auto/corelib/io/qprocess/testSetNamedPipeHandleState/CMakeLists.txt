#####################################################################
## testSetNamedPipeHandleState Binary:
#####################################################################

add_executable(testSetNamedPipeHandleState main.cpp)
target_link_libraries(testSetNamedPipeHandleState PRIVATE kernel32.lib)
