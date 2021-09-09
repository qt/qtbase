win32 {
    !winrt {
        SOURCES +=io/qstandardpaths_win.cpp 
    } else {
        SOURCES +=io/qstandardpaths_winrt.cpp
    }
} else:unix {
    mac {
        OBJECTIVE_SOURCES += io/qstandardpaths_mac.mm
    } else:android {
        SOURCES += io/qstandardpaths_android.cpp
    } else:haiku {
        SOURCES += io/qstandardpaths_haiku.cpp
    } else {
        SOURCES += io/qstandardpaths_unix.cpp
    }
}
