# special case begin
# special case skip regeneration
if (NOT WIN32)
    return()
endif()

qt_add_module(WinMain
  STATIC
  NO_SYNC_QT
  NO_MODULE_HEADERS
  DEFINES
      QT_NO_FOREACH
  INCLUDE_DIRECTORIES
      $<TARGET_PROPERTY:Qt::Core,INTERFACE_INCLUDE_DIRECTORIES>
)

qt_extend_target(WinMain CONDITION WINRT
    SOURCES qtmain_winrt.cpp
)

qt_extend_target(WinMain CONDITION NOT WINRT
    SOURCES qtmain_win.cpp
)

qt_extend_target(WinMain CONDITION MINGW
    DEFINES QT_NEEDS_QMAIN
)
# special case end
