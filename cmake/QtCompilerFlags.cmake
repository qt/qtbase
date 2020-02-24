# Set warnings. All compilers except MSVC support -Wall -Wextra
# Allow opting out by setting a QT_COMPILE_WARNINGS_OFF property on targets. This would be the
# equivalent of qmake's CONFIG += warn_off.

set(_qt_compiler_warning_flags_on "")
set(_qt_compiler_warning_flags_off "")

if (MSVC)
    list(APPEND _qt_compiler_warning_flags_on /W3)
    list(APPEND _qt_compiler_warning_flags_off -W0)
else()
    list(APPEND _qt_compiler_warning_flags_on -Wall -Wextra)
    list(APPEND _qt_compiler_warning_flags_off -w)
endif()

set(_qt_compiler_warning_flags_condition
    "$<BOOL:$<TARGET_PROPERTY:QT_COMPILE_OPTIONS_DISABLE_WARNINGS>>")
set(_qt_compiler_warning_flags_genex
    "$<IF:${_qt_compiler_warning_flags_condition},${_qt_compiler_warning_flags_off},${_qt_compiler_warning_flags_on}>")

# Need to replace semicolons so that the list is not wrongly expanded in the add_compile_options
# call.
string(REPLACE ";" "$<SEMICOLON>"
       _qt_compiler_warning_flags_genex "${_qt_compiler_warning_flags_genex}")
add_compile_options(${_qt_compiler_warning_flags_genex})
