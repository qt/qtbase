include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
#include <stdio.h>
#include <locale.h>

int main(int argc, char *argv[]) {
    _locale_t invalidLocale = NULL;
    double a = 3.14;
    const char *format = \"invalid format\";
    _sscanf_l(argv[0], invalidLocale, format, &a, &argc);
    _snprintf_l(argv[0], 1, invalidLocale, format, a);
}" HAVE__SPRINTF_L)

check_cxx_source_compiles("
#include <stdio.h>
#include <xlocale.h>

int main(int argc, char *argv[]) {
    locale_t invalidLocale = NULL;
    double a = 3.14;
    const char *format = \"invalid format\";
    snprintf_l(argv[0], 1, invalidLocale, format, a);
    sscanf_l(argv[0], invalidLocale, format, &a, &argc);
    return 0;
}" HAVE_SPRINTF_L)

add_library(WrapDoubleConversion INTERFACE)
if (NOT HAVE__SPRINTF_L AND NOT HAVE_SPRINTF_L)
    find_package(double-conversion)
    set_package_properties(double-conversion PROPERTIES TYPE REQUIRED)
    target_link_libraries(WrapDoubleConversion INTERFACE double-conversion::double-conversion)
endif()

set(WrapDoubleConversion_FOUND 1)

