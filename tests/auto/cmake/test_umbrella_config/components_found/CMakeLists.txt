
# The module finds its dependencies
find_package(Qt5 5.1.0
             COMPONENTS Core
    OPTIONAL_COMPONENTS DoesNotExist
)

if (NOT Qt5_FOUND)
    message(SEND_ERROR "Qt5 umbrella package not found!")
endif()

if (NOT Qt5Core_FOUND)
    message(SEND_ERROR "Qt5Core package not found!")
endif()

if (Qt5DoesNotExist_FOUND)
    message(SEND_ERROR "Non-existent package found!")
endif()
