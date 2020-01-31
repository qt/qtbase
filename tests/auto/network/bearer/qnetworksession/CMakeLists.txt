# Generated from qnetworksession.pro.

add_subdirectory(test)

if(NOT WINRT)
    add_subdirectory(lackey)
endif()
