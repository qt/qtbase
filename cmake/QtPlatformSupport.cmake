function(qt_set01 result)
    if (${ARGN})
        set("${result}" 1 PARENT_SCOPE)
    else()
        set("${result}" 0 PARENT_SCOPE)
    endif()
endfunction()

qt_set01(LINUX CMAKE_SYSTEM_NAME STREQUAL "Linux")
qt_set01(HPUX CMAKE_SYSTEM_NAME STREQUAL "HPUX")
qt_set01(ANDROID CMAKE_SYSTEM_NAME STREQUAL "Android")  # FIXME: How to identify this?
qt_set01(NACL CMAKE_SYSTEM_NAME STREQUAL "NaCl") # FIXME: How to identify this?
qt_set01(INTEGRITY CMAKE_SYSTEM_NAME STREQUAL "Integrity") # FIXME: How to identify this?
qt_set01(VXWORKS CMAKE_SYSTEM_NAME STREQUAL "VxWorks") # FIXME: How to identify this?
qt_set01(QNX CMAKE_SYSTEM_NAME STREQUAL "QNX") # FIXME: How to identify this?
qt_set01(OPENBSD CMAKE_SYSTEM_NAME STREQUAL "OpenBSD") # FIXME: How to identify this?
qt_set01(FREEBSD CMAKE_SYSTEM_NAME STREQUAL "FreeBSD") # FIXME: How to identify this?
qt_set01(NETBSD CMAKE_SYSTEM_NAME STREQUAL "NetBSD") # FIXME: How to identify this?
qt_set01(WASM CMAKE_SYSTEM_NAME STREQUAL "Emscripten")

qt_set01(BSD APPLE OR OPENBSD OR FREEBSD OR NETBSD)

qt_set01(WINRT WIN32 AND CMAKE_VS_PLATFORM_TOOSLET STREQUAL "winrt") # FIXME: How to identify this?

qt_set01(IOS APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
qt_set01(TVOS APPLE AND CMAKE_SYSTEM_NAME STREQUAL "tvOS")
qt_set01(WATCHOS APPLE AND CMAKE_SYSTEM_NAME STREQUAL "watchOS")
qt_set01(UIKIT APPLE AND (IOS OR TVOS OR WATCHOS))
qt_set01(MACOS APPLE AND NOT UIKIT)

qt_set01(GCC CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
qt_set01(CLANG CMAKE_CXX_COMPILER_ID MATCHES "Clang")
qt_set01(APPLECLANG CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
qt_set01(ICC CMAKE_C_COMPILER MATCHES "icc|icl")
qt_set01(QCC CMAKE_C_COMPILER MATCHES "qcc") # FIXME: How to identify this?

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(QT_64BIT TRUE)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(QT_32BIT TRUE)
endif()

# Parses a version string like "xx.yy.zz" and sets the major, minor and patch variables.
function(qt_parse_version_string version_string out_var_prefix)
    string(REPLACE "." ";" version_list ${version_string})
    list(LENGTH version_list length)

    set(out_var "${out_var_prefix}_MAJOR")
    set(value "")
    if(length GREATER 0)
        list(GET version_list 0 value)
        list(REMOVE_AT version_list 0)
        math(EXPR length "${length}-1")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)

    set(out_var "${out_var_prefix}_MINOR")
    set(value "")
    if(length GREATER 0)
        list(GET version_list 0 value)
        set(${out_var} "${value}" PARENT_SCOPE)
        list(REMOVE_AT version_list 0)
        math(EXPR length "${length}-1")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)

    set(out_var "${out_var_prefix}_PATCH")
    set(value "")
    if(length GREATER 0)
        list(GET version_list 0 value)
        set(${out_var} "${value}" PARENT_SCOPE)
        list(REMOVE_AT version_list 0)
        math(EXPR length "${length}-1")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Set up the separate version components for the compiler version, to allow mapping of qmake
# conditions like 'equals(QT_GCC_MAJOR_VERSION,5)'.
if(CMAKE_CXX_COMPILER_VERSION)
    qt_parse_version_string("${CMAKE_CXX_COMPILER_VERSION}" "QT_COMPILER_VERSION")
endif()
