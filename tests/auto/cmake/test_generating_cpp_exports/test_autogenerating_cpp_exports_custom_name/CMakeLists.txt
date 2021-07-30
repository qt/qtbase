qt_internal_add_module(TestAutogeneratingCppExportsCustomName
    GENERATE_CPP_EXPORTS
    CPP_EXPORT_HEADER_BASE_NAME
        "customname_exports"
    SOURCES
        module_api.h
        module_api.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)

qt_internal_extend_target(TestAutogeneratingCppExportsCustomName
    CONDITION GCC OR MINGW
    COMPILE_OPTIONS
        -fvisibility=hidden
)

add_executable(testapp2 use_api.cpp)
target_link_libraries(testapp2 PRIVATE TestAutogeneratingCppExportsCustomName)
