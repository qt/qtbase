# Generated from lib2.pro.

#####################################################################
## mylib Generic Library:
#####################################################################

qt_internal_add_cmake_library(mylib2 # special case
    SHARED
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}tst_qlibrary"
    #OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../" # special case
    SOURCES
        mylib.c
    PUBLIC_LIBRARIES
        Qt::Core
)

# special case begin
# This test is very "annoying" to get working with CMake as it involves having
# two targets with the same name on the parent scope, which is not possible with
# CMake. Furthermore, on UNIX, this version of the library should override the
# root symlink (libmylib.so) to point to version 2.
# Since we can't build two targets with the same name and in the same directory,
# we build mylib2 in it's own directory and manually copy and create the
# symlinks in the parent directory.
# Finally we also need to create a libmylib.so2 file in the parent directory.
#
set_target_properties(mylib2 PROPERTIES
    OUTPUT_NAME mylib
)
set_target_properties(mylib2 PROPERTIES
    VERSION 2.0.0
    SOVERSION 2
    C_VISIBILITY_PRESET "default"
    CXX_VISIBILITY_PRESET "default"
)

if(WIN32)
    # CMake sets for Windows-GNU platforms the suffix "lib"
    set_property(TARGET mylib2 PROPERTY PREFIX "")
endif()

if (UNIX)
    if(NOT ANDROID)
        add_custom_command(TARGET mylib2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mylib2>
                "${CMAKE_CURRENT_BINARY_DIR}/../system.qt.test.mylib.so"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mylib2>
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so.2.0.0"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.so.2.0.0"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so.2"
            COMMAND ${CMAKE_COMMAND} -E remove
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.so.2.0.0"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.so.2.0.0"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so2"
            VERBATIM)
    else()
        # Android does not use symlinks. Also, according to our conventions,
        # libraries on Android MUST be named in the following pattern:
        # lib*.so
        add_custom_command(TARGET mylib2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mylib2>
            "${CMAKE_CURRENT_BINARY_DIR}/../libsystem.qt.test.mylib.so"
            VERBATIM)
    endif()
else() #Win32
    add_custom_command(TARGET mylib2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mylib2>
            "${CMAKE_CURRENT_BINARY_DIR}/../system.qt.test.mylib.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mylib2>
            "${CMAKE_CURRENT_BINARY_DIR}/../mylib.dl2"
        VERBATIM)
endif()
# special case end

#### Keys ignored in scope 1:.:.:lib2.pro:<TRUE>:
# INSTALLS = "target" "renamed_target"
# QMAKE_POST_LINK = "$$member(QMAKE_POST_LINK, 0, -2)"
# TEMPLATE = "lib"
# VERSION = "2"
# renamed_target.extra = "$$member(renamed_target.extra, 0, -2)"
# renamed_target.path = "$$target.path"
# target.path = "$$[QT_INSTALL_TESTS]$${QMAKE_DIR_SEP}tst_qlibrary"

## Scopes:
#####################################################################

qt_internal_extend_target(mylib2 CONDITION MSVC # special case
    DEFINES
        WIN32_MSVC
)

#### Keys ignored in scope 3:.:.:lib2.pro:WIN32:
# DESTDIR = "../$$BUILD_FOLDER/"
# files = "$$BUILD_FOLDER$${QMAKE_DIR_SEP}mylib.dl2" "$$BUILD_FOLDER$${QMAKE_DIR_SEP}system.qt.test.mylib.dll"

#### Keys ignored in scope 4:.:.:lib2.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# BUILD_FOLDER = "debug"

#### Keys ignored in scope 5:.:.:lib2.pro:else:
# BUILD_FOLDER = "release"

#### Keys ignored in scope 6:.:.:lib2.pro:TEMPLATE___contains___vc._x_:
# src = "$(TargetPath)"

#### Keys ignored in scope 7:.:.:lib2.pro:else:
# src = "$(DESTDIR_TARGET)"

#### Keys ignored in scope 8:.:.:lib2.pro:else:
# files = "libmylib.so2" "system.qt.test.mylib.so"
# src = "$(DESTDIR)$(TARGET)"
