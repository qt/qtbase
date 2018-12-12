add_qt_executable(rasterwindow
  SOURCES
    main.cpp
    rasterwindow.cpp rasterwindow.h
  LIBRARIES Qt::Core Qt::Test Qt::Gui
)
