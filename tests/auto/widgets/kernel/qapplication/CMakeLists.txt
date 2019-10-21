# Generated from qapplication.pro.

add_subdirectory(test)
if(NOT WINRT)
    add_subdirectory(desktopsettingsaware)
    add_subdirectory(modal)
endif()
