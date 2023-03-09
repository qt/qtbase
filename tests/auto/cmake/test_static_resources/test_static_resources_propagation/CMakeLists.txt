# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# TODO: Revisit which of these tests makes sense to keep now that we depend on CMake 3.21 to
# properly place object libraries object files on the link line.
# See QTBUG-95601
#
#
#
#
# Add a dummy library that links the static "Qt" module containing resources
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp" CONTENT "void dummy() { }")
add_library(dummy STATIC "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp")
set_target_properties(dummy PROPERTIES UNITY_BUILD OFF)
target_link_libraries(dummy PRIVATE MockStaticResources1)

# Add the executable using qt_add_executable that needs to initialize the propagated resources.
# Finalize it implicitly(since CMake version 3.19).
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    qt_add_executable(test_static_resources_propagation main.cpp)
    set_target_properties(test_static_resources_propagation PROPERTIES
        AUTOMOC TRUE
        UNITY_BUILD OFF
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
    UNITY_BUILD OFF
)
target_link_libraries(test_static_resources_propagation_manual_finalize
    PRIVATE
        dummy
        Qt::Core
        Qt::Test
)
qt_finalize_target(test_static_resources_propagation_manual_finalize)
add_test(NAME test_static_resources_propagation_manual_finalize
    COMMAND test_static_resources_propagation_manual_finalize
)

# Add the executable using add_executable that needs to initialize the propagated resources.
# Finalize it explicitly.
add_executable(test_static_resources_propagation_non_qt main.cpp)
set_target_properties(test_static_resources_propagation_non_qt PROPERTIES
    AUTOMOC TRUE
    UNITY_BUILD OFF
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

get_target_property(link_order_matters
    ${QT_CMAKE_EXPORT_NAMESPACE}::Platform
    _qt_link_order_matters
)

if(NOT link_order_matters)
    # Add the executable using add_executable, expecting resources to be linked regardless of order.
    add_executable(test_static_resources_propagation_non_ld main.cpp)
    set_target_properties(test_static_resources_propagation_non_ld PROPERTIES
        AUTOMOC TRUE
        UNITY_BUILD OFF
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

# FIXME: qt6_enable_object_libraries_finalizer_mode is not available anymore. See QTBUG-95601
#
#    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
#        qt_add_executable(test_static_resources_propagation_not_finalize main.cpp)
#        qt6_enable_object_libraries_finalizer_mode(
#            test_static_resources_propagation_not_finalize FALSE
#        )
#        set_target_properties(test_static_resources_propagation_not_finalize PROPERTIES
#                AUTOMOC TRUE
#        )
#        target_link_libraries(test_static_resources_propagation_not_finalize
#            PRIVATE
#                dummy
#                Qt::Core
#                Qt::Test
#        )
#        add_test(NAME test_static_resources_propagation_not_finalize
#            COMMAND test_static_resources_propagation_not_finalize
#        )
#    endif()
endif()

# Add the executable using add_executable, expecting resources are propagated using
# target_link_options approach. The test is not applicable for qt_add_executable call since
# we use the CMP0099 policy NEW unless the actual version of CMake is lower than 3.17, that means
# target_link_options will always be preferable to finalizers.
if(POLICY CMP0099)
    cmake_policy(PUSH)

    cmake_policy(SET CMP0099 OLD)
    # When CMP0099 is set to OLD target_link_options doesn't propagate the linker options when
    # linking static libraries with a PRIVATE visibility but we finalize it explicitly. This
    # is a pure finalizer use case for platforms where link order matters.
    add_executable(test_static_resources_propagation_cmp0099_old_finalize main.cpp)
    set_target_properties(test_static_resources_propagation_cmp0099_old_finalize PROPERTIES
        AUTOMOC TRUE
        UNITY_BUILD OFF
    )
    target_link_libraries(test_static_resources_propagation_cmp0099_old_finalize
        PRIVATE
            dummy
            Qt::Core
            Qt::Test
    )
    qt_finalize_target(test_static_resources_propagation_cmp0099_old_finalize)
    add_test(NAME test_static_resources_propagation_cmp0099_old_finalize
        COMMAND test_static_resources_propagation_cmp0099_old_finalize
    )

    # When CMP0099 is set to NEW target_link_options propagates the linker options when linking
    # static libraries with a PRIVATE visibility. This is a pure target_link_options use case for
    # platforms where link order matters.
    cmake_policy(SET CMP0099 NEW)
    add_executable(test_static_resources_propagation_cmp0099_new main.cpp)
    set_target_properties(test_static_resources_propagation_cmp0099_new PROPERTIES
        AUTOMOC TRUE
        UNITY_BUILD OFF
    )
    target_link_libraries(test_static_resources_propagation_cmp0099_new
        PRIVATE
            dummy
            Qt::Core
            Qt::Test
    )
    add_test(NAME test_static_resources_propagation_cmp0099_new
        COMMAND test_static_resources_propagation_cmp0099_new
    )

    # Check if linking libraries using genex propagates resource objects when CMP0099 is enabled
    add_executable(test_static_resources_propagation_cmp0099_new_genex main.cpp)
    set_target_properties(test_static_resources_propagation_cmp0099_new_genex PROPERTIES
        AUTOMOC TRUE
        UNITY_BUILD OFF
    )
    target_link_libraries(test_static_resources_propagation_cmp0099_new_genex
        PRIVATE
            $<1:dummy>
            Qt::Core
            Qt::Test
    )
    add_test(NAME test_static_resources_propagation_cmp0099_new_genex
        COMMAND test_static_resources_propagation_cmp0099_new_genex
    )

    cmake_policy(POP)
endif()
