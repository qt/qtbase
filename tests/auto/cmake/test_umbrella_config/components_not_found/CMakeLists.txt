
# The module finds its dependencies
find_package(Qt6
    COMPONENTS Core DoesNotExist
)

if (Qt6_FOUND)
    message(SEND_ERROR "Qt6 umbrella package found, though it should not be!")
endif()

if (NOT Qt6Core_FOUND)
    message(SEND_ERROR "Qt6Core package not found!")
endif()

if (Qt6DoesNotExist_FOUND)
    message(SEND_ERROR "Non-existent package found!")
endif()
