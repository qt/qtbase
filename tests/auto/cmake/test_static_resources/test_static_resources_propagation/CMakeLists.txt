# Add a dummy library that links the static "Qt" module containing resources
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp" CONTENT "void dummy() { }")
add_library(dummy STATIC "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp")
target_link_libraries(dummy PRIVATE MockStaticResources1)


# Add the executable using qt_add_executable that needs to initialize the propagated resources.
# Finalize it implicitly(since CMake version 3.19).
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    qt_add_executable(test_static_resources_propagation main.cpp)
    set_target_properties(test_static_resources_propagation PROPERTIES
        AUTOMOC TRUE
    )
    target_link_libraries(test_static_resources_propagation
        PRIVATE
            dummy
            Qt::Core
            Qt::Test
    )
    add_test(NAME test_static_resources_propagation
        COMMAND test_static_resources_propagation
    )
endif()


# Add the executable using qt_add_executable that needs to initialize the propagated resources.
# Finalize it explicitly.
qt_add_executable(test_static_resources_propagation_manual_finalize main.cpp MANUAL_FINALIZATION)
set_target_properties(test_static_resources_propagation_manual_finalize PROPERTIES
    AUTOMOC TRUE
)
target_link_libraries(test_static_resources_propagation_manual_finalize
    PRIVATE
        dummy
        Qt::Core
        Qt::Test
)
add_test(NAME test_static_resources_propagation_manual_finalize
    COMMAND test_static_resources_propagation_manual_finalize
)
qt_finalize_target(test_static_resources_propagation_manual_finalize)


# Add the executable using add_executable that needs to initialize the propagated resources.
# Finalize it explicitly.
add_executable(test_static_resources_propagation_non_qt main.cpp)
set_target_properties(test_static_resources_propagation_non_qt PROPERTIES
    AUTOMOC TRUE
)
target_link_libraries(test_static_resources_propagation_non_qt
    PRIVATE
        dummy
        Qt::Core
        Qt::Test
)
qt_finalize_target(test_static_resources_propagation_non_qt)
add_test(NAME test_static_resources_propagation_non_qt
    COMMAND test_static_resources_propagation_non_qt
)


if(NOT GCC AND NOT MINGW AND NOT CLANG)
    ## Add the executable using add_executable, expecting resources to be linked regardless of order.
    add_executable(test_static_resources_propagation_non_ld main.cpp)
    set_target_properties(test_static_resources_propagation_non_ld PROPERTIES
        AUTOMOC TRUE
    )
    target_link_libraries(test_static_resources_propagation_non_ld
        PRIVATE
            dummy
            Qt::Core
            Qt::Test
    )
    add_test(NAME test_static_resources_propagation_non_ld
        COMMAND test_static_resources_propagation_non_ld
    )

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
        qt_add_executable(test_static_resources_propagation_not_finalize main.cpp)
        qt6_enable_resource_objects_finalizer_mode(
            test_static_resources_propagation_not_finalize FALSE
        )
        set_target_properties(test_static_resources_propagation_not_finalize PROPERTIES
                AUTOMOC TRUE
        )
        target_link_libraries(test_static_resources_propagation_not_finalize
            PRIVATE
                dummy
                Qt::Core
                Qt::Test
        )
        add_test(NAME test_static_resources_propagation_not_finalize
            COMMAND test_static_resources_propagation_not_finalize
        )
    endif()
endif()
