#! [qt5_wrap_ui]
set(SOURCES mainwindow.cpp main.cpp)
qt5_wrap_ui(SOURCES mainwindow.ui)
add_executable(myapp ${SOURCES})
#! [qt5_wrap_ui]

#! [qt_wrap_ui]
set(SOURCES mainwindow.cpp main.cpp)
qt_wrap_ui(SOURCES mainwindow.ui)
add_executable(myapp ${SOURCES})
#! [qt_wrap_ui]
