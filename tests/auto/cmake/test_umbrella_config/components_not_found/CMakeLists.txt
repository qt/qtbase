
# The module finds its dependencies
find_package(Qt5
    COMPONENTS Core DoesNotExist
)

if (Qt5_FOUND)
    message(SEND_ERROR "Qt5 umbrella package found, though it should not be!")
endif()

if (NOT Qt5Core_FOUND)
    message(SEND_ERROR "Qt5Core package not found!")
endif()

if (Qt5DoesNotExist_FOUND)
    message(SEND_ERROR "Non-existent package found!")
endif()
