#! [syntax]
<condition> {
    <command or definition>
    ...
}
#! [syntax]

#! [0]
win32 {
    SOURCES += paintwidget_win.cpp
}
#! [0]

#! [1]
!win32 {
    SOURCES -= paintwidget_win.cpp
}
#! [1]

unix {
    SOURCES += paintwidget_unix.cpp
}

#! [2]
macx {
    CONFIG(debug, debug|release) {
        HEADERS += debugging.h
    }
}
#! [2]

#! [3]
macx:CONFIG(debug, debug|release) {
    HEADERS += debugging.h
}
#! [3]

#! [4]
win32|macx {
    HEADERS += debugging.h
}
#! [4]

#! [5]
if(win32|macos):CONFIG(debug, debug|release) {
    # Do something on Windows and macOS,
    # but only for the debug configuration.
}
win32|if(macos:CONFIG(debug, debug|release)) {
    # Do something on Windows (regardless of debug or release)
    # and on macOS (only for debug).
}
#! [5]

#! [6]
win32-* {
    # Matches every mkspec starting with "win32-"
    SOURCES += win32_specific.cpp
}
#! [6]
