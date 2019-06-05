# special case begin
if (NOT WIN32)
    return()
endif()

add_qt_module(WinMain
  STATIC
  NO_MODULE_HEADERS
  DEFINES
      QT_NO_FOREACH
  INCLUDE_DIRECTORIES
      $<TARGET_PROPERTY:Qt::Core,INTERFACE_INCLUDE_DIRECTORIES>
)

extend_target(WinMain CONDITION WINRT
    SOURCES qtmain_winrt.cpp
)

extend_target(WinMain CONDITION NOT WINRT
    SOURCES qtmain_win.cpp
)

extend_target(WinMain CONDITION MINGW
    DEFINES QT_NEEDS_QMAIN
)

extend_target(Core LIBRARIES WinMain)
# special case end
