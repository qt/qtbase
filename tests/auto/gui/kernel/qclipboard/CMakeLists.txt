# Generated from qclipboard.pro.

add_subdirectory(test)
if(NOT WINRT)
    add_subdirectory(copier)
    add_subdirectory(paster)
endif()
