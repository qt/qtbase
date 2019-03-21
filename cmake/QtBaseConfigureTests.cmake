include(CheckCXXSourceCompiles)

function(run_config_test_architecture)
    # Test architecture
    set(_arch_file "${CMAKE_CURRENT_BINARY_DIR}/architecture_test")
    try_compile(_arch_result "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/arch/arch.cpp"
        COPY_FILE "${_arch_file}")
    if (NOT _arch_result)
        message(FATAL_ERROR "Failed to compile architecture detection file.")
    endif()

    file(STRINGS "${_arch_file}" _arch_lines LENGTH_MINIMUM 16 LENGTH_MAXIMUM 1024 ENCODING UTF-8)

    foreach (_line ${_arch_lines})
        string(FIND "${_line}" "==Qt=magic=Qt== Architecture:" _pos)
        if (_pos GREATER -1)
            math(EXPR _pos "${_pos}+29")
            string(SUBSTRING "${_line}" ${_pos} -1 _architecture)
        endif()
        string(FIND "${_line}" "==Qt=magic=Qt== Sub-architecture:" _pos)
        if (_pos GREATER -1)
            math(EXPR _pos "${_pos}+34")
            string(SUBSTRING "${_line}" ${_pos} -1 _sub_architecture)
            string(REPLACE " " ";" _sub_architecture "${_sub_architecture}")
        endif()
        string(FIND "${_line}" "==Qt=magic=Qt== Build-ABI:" _pos)
        if (_pos GREATER -1)
            math(EXPR _pos "${_pos}+26")
            string(SUBSTRING "${_line}" ${_pos} -1 _build_abi)
        endif()
    endforeach()

    if (NOT _architecture OR NOT _sub_architecture OR NOT _build_abi)
        message(FATAL_ERROR "Failed to extract architecture data from file.")
    endif()

    set(TEST_architecture 1 CACHE INTERNAL "Ran the architecture test")
    set(TEST_architecture_arch "${_architecture}" CACHE INTERNAL "Target machine architecture")
    set(TEST_subarch 1 CACHE INTERNAL "Ran machine subArchitecture test")
    foreach(it ${_sub_architecture})
        # Equivalent to qmake's QT_CPU_FEATURES.$arch.
        set(TEST_arch_${TEST_architecture_arch}_subarch_${it} 1 CACHE INTERNAL "Target sub architecture result")
    endforeach()
    set(TEST_buildAbi "${_build_abi}" CACHE INTERNAL "Target machine buildAbi")
endfunction()


function(run_config_test_posix_iconv)
    set(source "#include <iconv.h>

int main(int, char **)
{
    iconv_t x = iconv_open(\"\", \"\");

    char *inp;
    char *outp;
    size_t inbytes, outbytes;
    iconv(x, &inp, &inbytes, &outp, &outbytes);

    iconv_close(x);

    return 0;
}")
    check_cxx_source_compiles("${source}" HAVE_POSIX_ICONV)

    if(NOT HAVE_POSIX_ICONV)
        set(_req_libraries "${CMAKE_REQUIRE_LIBRARIES}")
        set(CMAKE_REQUIRE_LIBRARIES "iconv")
        check_cxx_source_compiles("${source}" HAVE_POSIX_ICONV)
        set(CMAKE_REQUIRE_LIBRARIES "${_req_libraries}")
        if(HAVE_POSIX_ICONV)
            set(TEST_iconv_needlib 1 CACHE INTERNAL "Need to link against libiconv")
        endif()
    endif()

    set(TEST_posix_iconv "${HAVE_POSIX_ICONV}" CACHE INTERNAL "POSIX iconv")
endfunction()


function(run_config_test_sun_iconv)
    set(source "#include <iconv.h>

int main(int, char **)
{
    iconv_t x = iconv_open(\"\", \"\");

    const char *inp;
    char *outp;
    size_t inbytes, outbytes;
    iconv(x, &inp, &inbytes, &outp, &outbytes);

    iconv_close(x);

    return 0;
}")
    if(DARWIN)
        # as per !config.darwin in configure.json
        set(HAVE_SUN_ICONV OFF)
    else()
        check_cxx_source_compiles("${source}" HAVE_SUN_ICONV)
    endif()

    set(TEST_sun_iconv "${HAVE_SUN_ICONV}" CACHE INTERNAL "SUN libiconv")
endfunction()

function(run_linker_version_script_support)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version_flag.map" "VERS_1 { global: sym; };
VERS_2 { global: sym; }
VERS_1;
")
    if(DEFINED CMAKE_REQUIRED_FLAGS)
        set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
    else()
        set(CMAKE_REQUIRED_FLAGS "")
    endif()
    set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} "-Wl,--version-script=\"${CMAKE_CURRENT_BINARY_DIR}/version_flag.map\"")
    check_cxx_source_compiles("int main(void){return 0;}" HAVE_LD_VERSION_SCRIPT)
    if(DEFINED CMAKE_REQUIRED_FLAGS_SAVE)
        set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})
    endif()
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/conftest.map")

    set(TEST_ld_version_script "${HAVE_LD_VERSION_SCRIPT}" CACHE INTERNAL "linker version script support")
endfunction()

function(run_config_tests)
    run_config_test_posix_iconv()

    add_library(Iconv INTERFACE)
    if(TEST_iconv_needlib)
       target_link_libraries(Iconv PUBLIC iconv)
    endif()

    if(NOT TEST_posix_iconv)
        run_config_test_sun_iconv()
    endif()
    run_config_test_architecture()
    run_linker_version_script_support()
endfunction()

run_config_tests()
