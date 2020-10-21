# special case begin
# special case skip regeneration
if (NOT WIN32)
    return()
endif()

qt_internal_add_module(EntryPoint
  STATIC
  INTERNAL_MODULE
  NO_SYNC_QT
  NO_MODULE_HEADERS
  NO_PRIVATE_MODULE
  DEFINES
      QT_NO_FOREACH
  INCLUDE_DIRECTORIES
      $<TARGET_PROPERTY:Qt::Core,INTERFACE_INCLUDE_DIRECTORIES>
)

if (MSVC)
    # Store debug information inside the static lib
    qt_internal_replace_compiler_flags(
        "/Zi" "/Z7"
        CONFIGS DEBUG RELWITHDEBINFO
        IN_CURRENT_SCOPE)
endif()

qt_internal_extend_target(EntryPoint CONDITION WIN32
    SOURCES qtentrypoint_win.cpp
    LIBRARIES shell32
)

qt_internal_extend_target(EntryPoint CONDITION MINGW
    DEFINES QT_NEEDS_QMAIN
)
# special case end
